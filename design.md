\# PML (Picture Markup Language) 语法规范



\*\*版本\*\*：0.1  

\*\*状态\*\*：草案  

\*\*基础语法\*\*：Lisp 风格（S‑表达式）



\---



\## 1. 概述



PML 是一种用于描述静态图像、动画、骨骼及逆向动力学的声明式/过程式语言。其设计目标包括：  

\- \*\*LLM 友好\*\*：语法简单，括号平衡，模块化清晰。  

\- \*\*表达力强\*\*：支持函数、宏、仿射变换、关键帧动画、IK 求解。  

\- \*\*易实现\*\*：核心解释器可在数千行 Python 内完成。



\---



\## 2. 词法



\### 2.1 注释

\- 行注释：`;` 开始直到行尾



\### 2.2 原子 (Atom)

| 类型 | 示例 |

|------|------|

| 整数 | `42`, `-10` |

| 浮点数 | `3.14`, `-0.5` |

| 字符串 | `"hello world"`（支持 `\\"` 转义） |

| 符号 | `circle`, `my-var`, `+`, `->`（不能以数字开头，可含 `-?\*+<=>!`） |

| 布尔 | `#t`, `#f` |

| 关键字 | `:fill`, `:stroke`, `:transform`（以冒号开头，用作函数参数标记） |



\### 2.3 列表

\- `( … )` 内可含任意原子或列表，元素之间由空白（空格、换行）分隔。



\### 2.4 空白

空格、制表符、换行符均被忽略（除了分隔元素）。



\---



\## 3. 核心特殊形式



解释器必须实现以下特殊形式（即语法关键字）。



| 形式 | 说明 |

|------|------|

| `(quote <expr>)` 或 `'<expr>` | 返回表达式本身，不求值 |

| `(if <cond> <then> <else>)` | 条件求值 |

| `(define <name> <expr>)` | 定义全局变量或函数（见第4节） |

| `(lambda <params> <body>)` | 创建匿名函数，`<params>` 为符号列表 |

| `(begin <expr> ...)` | 顺序求值，返回最后一个表达式的值 |

| `(set! <name> <expr>)` | 改变已定义变量的值 |

| `(load <filename>)` | 加载并执行当前目录下的 PML 文件（不提供模块隔离） |

| `(provide <symbol> ...)` | 导出当前模块的符号（用于模块系统） |

| `(import <filename> \[as <prefix>])` | 导入模块，返回模块对象（见第5节） |



\---



\## 4. 函数定义



\### 4.1 语法糖

```lisp

(define (name <params>) <body>)

```

等价于：

```lisp

(define name (lambda (<params>) <body>))

```



\### 4.2 示例

```lisp

(define (square x) (\* x x))

(define add (lambda (a b) (+ a b)))

```



\### 4.3 可变参数（可选）

```lisp

(define (printf fmt . args) ...)   ; args 为列表

```



\---



\## 5. 模块系统



\- 每个 `.pml` 文件是一个模块。

\- `import` 在当前环境下加载目标文件，并返回一个\*\*模块对象\*\*（一个类似于环境的作用域）。

\- 模块内的顶层定义默认\*\*不\*\*对外可见，必须通过 `provide` 显式导出。

\- 导入方使用 `prefix/symbol` 访问导出符号。



\### 5.1 示例

\*\*lib/math.pml\*\*：

```lisp

(provide clamp lerp)



(define (clamp x min max)

&#x20; (if (< x min) min (if (> x max) max x)))



(define (lerp a b t)

&#x20; (+ a (\* t (- b a))))

```



\*\*main.pml\*\*：

```lisp

(import "lib/math.pml" as math)

(define x (math/clamp 5 0 10))

(define y (math/lerp 0 100 0.5))

```



\### 5.2 模块加载规则

\- 路径相对于当前文件所在目录，支持 `"subdir/file.pml"`。

\- 不提供自动路径搜索（除非用户设置 `PML\_PATH` 环境变量，解释器可支持）。

\- 循环依赖检测：如果 A 导入 B 且 B 直接/间接导入 A，则抛出 `CircularImportError`。

\- 同一模块多次导入仅执行一次，返回缓存的模块对象。



\---



\## 6. 图形图元（内置函数）



所有图元函数返回一个“图形对象”（可被组合、变换、动画）。



| 函数 | 参数 | 关键字参数 |

|------|------|-------------|

| `(circle x y r)` | 圆心坐标，半径 | `:fill`, `:stroke`, `:stroke-width`, `:transform` |

| `(rect x y w h)` | 左上角坐标，宽高 | 同上 |

| `(ellipse cx cy rx ry)` | 中心坐标，x半径，y半径 | 同上 |

| `(line x1 y1 x2 y2)` | 起点、终点 | `:stroke`, `:stroke-width`, `:transform` |

| `(polygon points)` | 点列表 `((x1 y1)(x2 y2)...)` | 同上 |

| `(path d-string)` | SVG 路径字符串 | `:fill`, `:stroke`, `:transform` |

| `(text str x y)` | 字符串，左下角坐标 | `:font-family`, `:font-size`, `:fill`, `:transform` |

| `(image src x y \[w h])` | 图片路径，位置，可选宽高 | `:transform` |



\*\*关键字参数详解\*\*：

\- `:fill` / `:stroke` ：颜色值（支持 `"#RRGGBB"`, `"red"`, `"rgb(255,0,0)"`）

\- `:stroke-width` ：数字（默认1）

\- `:transform` ：变换矩阵（见第7节）

\- 其他后端特定参数可扩展，但建议保持跨后端一致。



\### 6.1 组合

```lisp

(group <graphic-expr> ...)

```

返回一个图形对象，内部顺序绘制所有子图形。



\### 6.2 画布与渲染

```lisp

(canvas <width> <height> \[:bg <color>])

```

设置画布大小及背景色（可选，默认透明或白色）。



```lisp

(render <filename> \[:format <symbol>] \[:fps <number>])

```

\- `format` 可为 `'png`, `'jpg`, `'gif`, `'mp4`（根据扩展名自动判断，也可强制指定）。

\- `fps` 仅对动画有效（默认30）。



\---



\## 7. 变换矩阵



\### 7.1 矩阵表示

一个仿射矩阵由6个数字 `(a b c d e f)` 表示，满足：

```

x' = a\*x + c\*y + e

y' = b\*x + d\*y + f

```



\### 7.2 矩阵构造与操作

```lisp

(transform a b c d e f)       ; 直接创建矩阵

(translate dx dy)              ; 返回平移矩阵

(rotate angle)                 ; 返回旋转矩阵（弧度）

(scale sx sy)                  ; 返回缩放矩阵

(shear shx shy)                ; 返回错切矩阵

(compose m1 m2)                ; 返回 m1 ∘ m2 (先应用 m2，再 m1)

(identity-matrix)              ; 单位矩阵

(matrix? obj)                  ; 判断是否为矩阵

```



\### 7.3 应用到图形对象

任何图元或 `group` 都可接受 `:transform` 关键字：

```lisp

(circle 0 0 10 :transform (translate 100 200))

```



\### 7.4 复合变换示例

```lisp

(define m (compose (translate 50 0) (rotate 0.785) (scale 2 2)))

(rect 0 0 100 100 :transform m)

```



\---



\## 8. 动画系统



\### 8.1 属性动画

```lisp

(animate <target> <property> <from> <to> <duration> 

&#x20;        \[:ease <symbol>] \[:repeat <integer-or-symbol>] \[:delay <number>])

```

\- `<target>` ：图形对象、矩阵、数字变量等可变对象。

\- `<property>` ：符号，如 `'x`, `'y`, `'fill`, `'transform`。

\- `<from>/<to>` ：起始/结束值。

\- `<duration>` ：秒（浮点数）。

\- `:ease` ：`'linear`, `'quad-in`, `'quad-out`, `'quad-in-out`, `'bounce` 等（标准缓动函数名）。

\- `:repeat` ：整数（重复次数）或 `'infinite`。

\- `:delay` ：动画开始前的延迟秒数。



\*\*返回值\*\*：动画对象（可用于控制）。



\### 8.2 颜色插值

当 `:fill` 或 `:stroke` 为颜色字符串时，自动在 RGB 空间线性插值。



\### 8.3 组合动画

```lisp

(parallel <anim> ...)   ; 同时播放

(sequence <anim> ...)   ; 顺序播放

```



\### 8.4 动画控制

```lisp

(play <anim>)

(stop <anim>)

(pause <anim>)

(seek <anim> <time>)

(on-finished <anim> <callback>)   ; callback 是无参函数

```



\### 8.5 每一帧钩子

```lisp

(every-frame <thunk>)

```

注册一个函数，在每帧渲染前调用（常用于 IK 求解更新）。



\---



\## 9. 骨骼与逆向动力学 (IK)



\### 9.1 定义骨骼模板

```lisp

(defskeleton <name> (<root-x> <root-y>)

&#x20; (joint <joint-name> :pos (<dx> <dy>) :length <len> :angle <init-angle>

&#x20;        \[:min <min-angle>] \[:max <max-angle>])

&#x20; ...)

```

\- `<dx> <dy>` ：相对于父关节的偏移（根关节相对于骨骼原点）。

\- `:angle` ：初始角度（弧度）。

\- `:min` / `:max` ：关节活动范围（弧度），可选。



示例：

```lisp

(defskeleton arm (x y)

&#x20; (joint shoulder :pos (0 0) :length 30 :angle -0.5 :min -1.5 :max 1.5)

&#x20; (joint elbow :pos (30 0) :length 30 :angle 0.3 :min 0 :max 2.0)

&#x20; (joint wrist :pos (30 0) :length 10 :angle 0))

```



\### 9.2 实例化骨骼

```lisp

(instantiate-skeleton <skeleton-template> (<root-x> <root-y>) \[:name <string>])

```

返回骨骼实例对象（包含每个关节的可变角度）。



\### 9.3 IK 求解

```lisp

(ik-solve <skeleton-instance> <end-effector-name> <target-x> <target-y>

&#x20;         \[:method 'fabrik] \[:iterations 10] \[:tolerance 0.01])

```

\- `method` ：`'fabrik`（推荐）或 `'ccd`。

\- 求解器直接修改实例中各关节的角度，以满足末端位置。



\### 9.4 绑定皮肤

```lisp

(bind-skin <graphic-object> <skeleton-instance> <joint-name> ...)

```

将图形对象的局部坐标系附加到指定的关节上。可绑定一个皮肤到多个关节（例如手臂组依次绑定到肩、肘、腕），此时图形对象会根据关节链的相对变换自动变形。



\### 9.5 获取关节全局位置

```lisp

(joint-position <skeleton-instance> <joint-name>)

```

返回 `(x y)` 全局坐标，用于绘制特效或约束。



\---



\## 10. 宏



\### 10.1 定义宏

```lisp

(defmacro <name> (<params>) <body>)

```

宏在求值前展开，参数是原始 S‑表达式（不求值）。展开结果被替换到原代码位置。



\### 10.2 示例

```lisp

(defmacro swap (a b)

&#x20; `(let ((tmp ,a))

&#x20;    (set! ,a ,b)

&#x20;    (set! ,b tmp)))

```

注意：使用反引号 ` 和逗号 `,` 进行模板引用（与 Scheme 相同）。



\### 10.3 卫生宏（可选）

初期实现非卫生宏，但建议使用 `gensym` 避免捕获：

```lisp

(defmacro repeat (n . body)

&#x20; (let ((i (gensym)))

&#x20;   `(do ((,i 0 (+ ,i 1))) ((>= ,i ,n)) ,@body)))

```



\---



\## 11. 外部资源



\### 11.1 图片与字体

\- 图片：`(image "path/to/file.png" x y)`。支持常见格式（PNG, JPEG, WebP）。

\- 字体：`(text "Hi" x y :font-family "path/to/font.ttf")`。解释器应缓存加载的字体/图片资源。



\### 11.2 资源搜索路径

解释器可提供一个变量 `\*resource-path\*`（默认包含当前目录及用户自定义目录），用于解析相对路径。



\### 11.3 缺失资源处理

\- 若文件不存在，抛出 `ResourceNotFoundError`。

\- 在宽容模式下，可用占位符（如棋盘格）替代图片，并打印警告。



\---



\## 12. 并发（可选）



\### 12.1 并行求值

```lisp

(parallel-eval <expr> ...)

```

同时求值多个独立表达式，返回列表。解释器可以使用线程池。



\### 12.2 动画与渲染的并发

\- 动画更新和 IK 求解可以放在后台线程，与 IO（渲染输出）重叠。

\- 实现方式：每帧先并行求解所有 IK，再收集绘图命令。



\### 12.3 注意事项

\- 图形对象不是线程安全的。若多个线程修改同一对象，需加锁（或设计为不可变）。

\- 初期实现可忽略并发，仅预留接口。



\---



\## 13. 错误处理



\### 13.1 异常类型

\- `SyntaxError` : 括号不匹配、非法原子。

\- `ImportError` : 文件未找到、循环依赖。

\- `TypeError` : 参数数量/类型错误。

\- `ResourceError` : 图片/字体缺失。

\- `IKNoSolutionError` : IK 求解无法收敛到容忍度内。



\### 13.2 模式

\- \*\*严格模式\*\*（默认）：任何错误立即中止执行并抛出异常，附带行号。

\- \*\*宽容模式\*\*：通过 `(set! \*safe-mode\* #t)` 开启。遇到可恢复错误（如缺失图片）时使用占位符并继续执行，不可恢复错误则中止。



\### 13.3 错误报告格式

为了便于 LLM 修正，错误输出应为结构化文本，至少包含：

```

Error: <type> at <file>:<line>

Message: ...

Context: ...（附近代码）

```



\---



\## 14. 完整示例：挥手机器人



```lisp

; skeletons/arm.skel

(defskeleton arm (x y)

&#x20; (joint shoulder :pos (0 0) :length 30 :angle 0 :min -1.4 :max 1.4)

&#x20; (joint elbow :pos (30 0) :length 25 :angle 0 :min 0 :max 2.0)

&#x20; (joint hand :pos (25 0) :length 0))



; characters/robot.pml

(import "skeletons/arm.skel" as skel)



(define (make-robot x y)

&#x20; (let ((left-arm (instantiate-skeleton skel/arm (x y)))

&#x20;       (body (rect (- x 20) (- y 40) 40 60 :fill "gray")))

&#x20;   (define (wave duration)

&#x20;     (animate left-arm (hand-x (+ x 30) (+ x 80) duration)

&#x20;              (hand-y y (- y 20) duration)

&#x20;              :repeat 3))

&#x20;   (lambda (cmd . args)

&#x20;     (if (eq? cmd 'wave) (wave (car args)) (error "unknown cmd")))))



; main.pml

(canvas 800 600 :bg "#f0f0f0")

(define robot (make-robot 400 400))

(robot 'wave 1.0)

(render "wave.gif" :format 'gif :duration 2.0)

```



\---



\## 15. 实现说明



\- 解释器应实现 \*\*REPL\*\* 或直接执行文件的模式。

\- 所有内置函数（图元、矩阵、动画、IK）由宿主语言（Python）提供并绑定到符号。

\- 标准库（`math.pml`, `color.pml` 等）随解释器发布，可在 `PML\_PATH` 中找到。

\- 推荐使用 `pytest` 测试核心功能。



\---



\*\*本规范为最终设计文档的起点，欢迎根据实现反馈修订。\*\*

