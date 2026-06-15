# PML 开发与架构文档

**版本**：0.1.0 | **状态**：草案 | **实现语言**：Python 3.10+


---

## 第一部分：项目概述

### 1.1 项目愿景

PML（Picture Markup Language）是一种面向 **sprite 资产生成** 的声明式/过程式编程语言。它以 Lisp 风格的 S-表达式为基础语法，核心使命是让**大语言模型（LLM）智能体**能够通过生成结构化代码来生产可用的游戏/动漫 sprite——包括角色、物品、UI 控件、场景等。

传统工作流中，LLM 生成图像依赖扩散模型，产出结果不可控、难以精确编辑、无法保证风格一致性。PML 提供了另一条路径：LLM 不再"画"图，而是"装配"图——通过组合参数化的语义组件（如 `(anime-eyes :style "shoujo" :color "#4a90d9")`）来描述 sprite，PML 解释器负责将其精确渲染为游戏引擎可直接使用的 sprite 资产。

这意味着 PML 的架构分为两层：底层是通用的几何图元与渲染管线（`circle`、`path`、`group` 等），上层是面向 sprite 领域的**语义组件库**（角色部件、UI 控件、场景元素等）。LLM 主要在语义层工作，底层供高级用户精细调控。

### 1.2 设计哲学

PML 的设计遵循以下原则，当出现取舍时，按以下优先级排序：

**LLM 装配优先于自由绘制**。PML 的首要用户是 LLM 智能体，而非人类画师。语义组件库应设计为"受限的生成空间"——参数范围经过精心约束，使得 LLM 在合法参数空间内的任意组合都不会产生"坏掉"的 sprite。底层几何图元保留给高级用户微调。

**可判定性优先于灵活性**。语法应当足够简单，使得括号匹配、语法错误可以被静态检测。不引入隐式行为——所有副作用必须显式表达。

**组合性优先于完备性**。与其为每种场景提供专用语法，不如提供可组合的原语。底层有 `group` + `transform`，语义层有可插拔的角色部件、UI 控件、场景元素——都遵循组合优于枚举的理念。

**渐进复杂度**。简单程序应当简单编写：`(circle 0 0 10 :fill "red")` 即可画一个圆，`(character :hair "spiky" :eyes "round")` 即可生成一个角色。高级功能（宏、IK、并发）是可选项，不应增加基础用法的心智负担。

**sprite 即资产**。输出不是"一张图"，而是游戏引擎可直接消费的资产。透明通道、锚点、画框规格、sprite sheet 排列是一等公民，而非后期处理的附加项。

**实现透明**。核心解释器应在数千行 Python 内完成，不依赖复杂的解析器生成器或编译框架。任何阅读源码的开发者都应能在合理时间内理解全貌。

### 1.3 项目目标

| 维度 | 目标 |
|------|------|
| 语法 | Lisp S-表达式，括号平衡，无隐式规则 |
| 表达力 | 函数、宏、仿射变换、关键帧动画、IK 求解 |
| 语义组件层 | 角色（身体/头发/眼睛/服装/表情）、物品、UI 控件、场景元素 |
| 风格系统 | 预定义风格模板（赛璐珞/像素风/水彩等），风格一致性约束 |
| sprite 输出 | PNG（透明通道）、sprite sheet、固定画框、锚点定义、多分辨率 |
| 动画输出 | GIF、MP4、帧序列、骨骼动画 |
| 实现规模 | 核心解释器 ≤ 5000 行 Python（不含组件库与后端渲染） |
| 标准库 | 基础模块（`math.pml`、`color.pml`）+ 语义组件库（`stdlib/sprites/`） |
| 运行模式 | 文件执行模式 + REPL + LLM API 接口 |


---

## 第二部分：语言语法规范

### 2.1 词法结构

PML 的词法分析将源文本分解为 Token 序列。Token 类型如下：

#### 2.1.1 注释

以分号 `;` 开始，延续到行尾。注释在词法分析阶段被完全丢弃，不进入 Token 流。

```pml
; 这是一行注释
(circle 0 0 10) ; 行尾注释也是合法的
```

嵌套注释（如 `#| ... |#`）不在初始规范中，但词法分析器应预留扩展点。

#### 2.1.2 原子 (Atom)

| 类型 | 正则模式 | 示例 | 内部表示 |
|------|---------|------|---------|
| 整数 | `[+-]?\d+` | `42`, `-10`, `+7` | Python `int` |
| 浮点数 | `[+-]?\d+\.\d*` 或 `[+-]?\.\d+` | `3.14`, `-0.5`, `.25` | Python `float` |
| 字符串 | `"` 开头，支持 `\"` `\\` 转义 | `"hello"`, `"a\"b"` | Python `str` |
| 符号 | 不以数字开头，可含字母、数字、`-?*+<=>!_/` | `circle`, `my-var`, `+`, `->` | Python `str`（ intern ） |
| 布尔 | `#t`, `#f` | `#t`, `#f` | Python `True` / `False` |
| 关键字 | 以冒号 `:` 开头的符号 | `:fill`, `:stroke`, `:transform` | Python `str`（带 `:` 前缀） |

**符号的合法性规则**：不能以数字或 `"` 开头；不能包含空白字符、括号、分号；保留字符 `'`、`` ` ``、`,` 不作为符号的一部分（它们是语法糖的前缀）。

**关键字的特殊地位**：关键字（Keyword）本质上是以冒号开头的符号，但在函数调用中扮演"命名参数标记"的角色。当解释器遇到关键字时，它将后续的表达式作为该参数的值：

```pml
(circle 0 0 10 :fill "red" :stroke "blue" :stroke-width 2)
; 等价于位置参数 + 命名参数的混合调用
```

#### 2.1.3 列表 (List)

由圆括号 `( )` 包裹的 Token 序列，元素之间由空白分隔：

```pml
(+ 1 2)
(circle 0 0 10 :fill "red")
(define (square x) (* x x))
```

列表可以嵌套，且嵌套深度不设上限（受限于 Python 递归栈）：

```pml
(group
  (circle 0 0 10)
  (group
    (rect 0 0 20 20)
    (line 0 0 20 20)))
```

#### 2.1.4 语法糖前缀

| 前缀 | 展开形式 | 示例 |
|------|---------|------|
| `'expr` | `(quote expr)` | `'(1 2 3)` → `(quote (1 2 3))` |
| `` `expr `` | `(quasiquote expr)` | `` `(a ,b c) `` → `(quasiquote (a (unquote b) c))` |
| `,expr` | `(unquote expr)` | 仅在 quasiquote 内有效 |
| `,@expr` | `(unquote-splicing expr)` | 仅在 quasiquote 内有效 |

#### 2.1.5 空白

空格（` `）、制表符（`\t`）、换行符（`\n`、`\r\n`）均作为分隔符，在词法分析阶段被折叠为单一分隔标记。多个连续空白等价于一个。

### 2.2 类型系统

PML 采用动态类型。运行时值的类型如下：

| 类型 | 说明 | Python 对应 |
|------|------|------------|
| `Number` | 整数或浮点数 | `int` / `float` |
| `String` | UTF-8 字符串 | `str` |
| `Boolean` | 布尔值 | `bool` |
| `Symbol` | 符号（intern 字符串） | `str` |
| `Keyword` | 关键字 | `str`（以 `:` 开头） |
| `List` | 有序序列 | `list` |
| `Procedure` | 函数（lambda 或内置） | 自定义 `Procedure` 类 |
| `Macro` | 宏 | 自定义 `Macro` 类 |
| `GraphicObject` | 图形对象 | 自定义 `GraphicObject` 类 |
| `Transform` | 仿射变换矩阵 | 自定义 `AffineTransform` 类 |
| `Animation` | 动画对象 | 自定义 `Animation` 类 |
| `Skeleton` | 骨骼模板 | 自定义 `SkeletonTemplate` 类 |
| `SkeletonInstance` | 骨骼实例 | 自定义 `SkeletonInstance` 类 |
| `Module` | 模块对象 | 自定义 `Module` 类 |
| `Nil` | 空值 | `None` |

类型检查在运行时进行。当函数收到不兼容的参数类型时，抛出 `TypeError`，并附带期望类型和实际类型信息。

### 2.3 核心特殊形式

特殊形式是解释器内置的语法关键字，其求值规则不同于普通函数调用——参数的求值时机由特殊形式自行控制。

#### `(quote <expr>)` 或 `'<expr>`

返回表达式的字面形式，不求值。这是构造列表数据和符号的基本手段。

```pml
(quote (1 2 3))     ; => (1 2 3) 作为列表值
(quote hello)       ; => 符号 hello
'(a b c)            ; => (a b c)，等价写法
```

#### `(if <cond> <then> <else>)`

条件表达式。先求值 `<cond>`，若为真值（非 `#f`、非 `0`、非 `nil`），则求值 `<then>`；否则求值 `<else>`。`<then>` 和 `<else>` 不会同时被求值。

```pml
(if (> x 0) "positive" "non-positive")
```

#### `(cond (<test1> <expr1>) (<test2> <expr2>) ... (else <default>))`

多分支条件，可展开为嵌套的 `if`。`else` 分支可选。

```pml
(cond
  ((> x 0)  "positive")
  ((< x 0)  "negative")
  (else     "zero"))
```

#### `(define <name> <expr>)`

在当前环境中绑定符号 `<name>` 到 `<expr>` 的值。若 `<name>` 已存在，则覆盖（仅在同一作用域内）。

函数定义的语法糖：

```pml
(define (square x) (* x x))
; 等价于
(define square (lambda (x) (* x x)))
```

#### `(lambda <params> <body>)`

创建匿名函数。`<params>` 是符号列表，`<body>` 是函数体（多条表达式时隐式 `begin`）。

```pml
(lambda (x y) (+ x y))

; 可变参数
(lambda (fmt . args) ...)
; args 收集剩余实参为列表
```

闭包：`lambda` 捕获定义时的环境，形成闭包。闭包内的自由变量引用外层环境中的绑定。

#### `(let ((<name1> <expr1>) (<name2> <expr2>) ...) <body>)`

创建局部作用域，将 `<name>` 绑定到对应 `<expr>` 的值，然后在扩展环境中求值 `<body>`。所有 `<expr>` 在外层环境中求值（非递归 let）。

```pml
(let ((x 10)
      (y 20))
  (+ x y))  ; => 30
```

#### `(let* ((<name1> <expr1>) ...) <body>)`

与 `let` 类似，但绑定是顺序的——后续绑定可以引用前面的绑定。

```pml
(let* ((x 10)
       (y (* x 2)))  ; y 可以引用 x
  y)  ; => 20
```

#### `(begin <expr> ...)`

顺序求值所有表达式，返回最后一个表达式的值。常用于需要副作用序列的上下文。

```pml
(begin
  (set! x (+ x 1))
  (set! y (+ y 1))
  (+ x y))
```

#### `(set! <name> <expr>)`

修改已有绑定的值。`<name>` 必须在当前或外层环境中已定义，否则抛出 `UnboundVariableError`。

```pml
(define x 10)
(set! x 20)  ; x 现在是 20
```

#### `(do ((<var> <init> <step>) ...) (<test> <result>) <body>)`

迭代结构。先初始化变量，然后循环：每轮先检查 `<test>`，若为真则返回 `<result>`；否则执行 `<body>`，再按 `<step>` 更新变量。

```pml
(do ((i 0 (+ i 1))
     (sum 0 (+ sum i)))
    ((>= i 10) sum))
; => 45
```

### 2.4 标准库函数

以下是解释器必须内置的基础函数，它们在启动时自动注入全局环境。

#### 算术运算

| 函数 | 签名 | 说明 |
|------|------|------|
| `+` | `(+ n ...) → Number` | 求和，无参数返回 0 |
| `-` | `(- n1 n2 ...) → Number` | 减法；单参数时返回相反数 |
| `*` | `(* n ...) → Number` | 求积，无参数返回 1 |
| `/` | `(/ n1 n2 ...) → Number` | 除法；单参数时返回倒数 |
| `%` | `(% n1 n2) → Number` | 取模 |
| `abs` | `(abs n) → Number` | 绝对值 |
| `min` | `(min n ...) → Number` | 最小值 |
| `max` | `(max n ...) → Number` | 最大值 |
| `floor` | `(floor n) → Integer` | 向下取整 |
| `ceil` | `(ceil n) → Integer` | 向上取整 |
| `round` | `(round n) → Integer` | 四舍五入 |
| `sqrt` | `(sqrt n) → Number` | 平方根 |
| `pow` | `(pow base exp) → Number` | 幂运算 |
| `sin` | `(sin rad) → Number` | 正弦 |
| `cos` | `(cos rad) → Number` | 余弦 |
| `tan` | `(tan rad) → Number` | 正切 |
| `atan2` | `(atan2 y x) → Number` | 反正切（四象限） |

#### 比较运算

| 函数 | 签名 | 说明 |
|------|------|------|
| `=` | `(= a b) → Boolean` | 数值相等 |
| `<` | `(< a b) → Boolean` | 小于 |
| `>` | `(> a b) → Boolean` | 大于 |
| `<=` | `(<= a b) → Boolean` | 小于等于 |
| `>=` | `(>= a b) → Boolean` | 大于等于 |
| `eq?` | `(eq? a b) → Boolean` | 引用相等（符号、布尔） |
| `equal?` | `(equal? a b) → Boolean` | 结构相等（深度比较） |

#### 逻辑运算

| 函数 | 说明 |
|------|------|
| `(and <expr> ...)` | 短路求值，遇 `#f` 立即返回 |
| `(or <expr> ...)` | 短路求值，遇真值立即返回 |
| `(not <expr>)` | 逻辑非 |

注意：`and` 和 `or` 是特殊形式而非函数——它们支持短路求值。

#### 列表操作

| 函数 | 签名 | 说明 |
|------|------|------|
| `cons` | `(cons a b) → List` | 构造序对 |
| `car` | `(car lst) → Any` | 取列表第一个元素 |
| `cdr` | `(cdr lst) → List` | 取列表除首元素外的剩余部分 |
| `list` | `(list a ...) → List` | 构造列表 |
| `length` | `(length lst) → Number` | 列表长度 |
| `append` | `(append lst ...) → List` | 拼接列表 |
| `reverse` | `(reverse lst) → List` | 反转列表 |
| `map` | `(map fn lst) → List` | 映射 |
| `filter` | `(filter fn lst) → List` | 过滤 |
| `reduce` | `(reduce fn init lst) → Any` | 归约 |
| `nth` | `(nth lst i) → Any` | 按索引取值 |
| `range` | `(range start end [step]) → List` | 生成整数序列 |
| `null?` | `(null? x) → Boolean` | 是否为空列表 |
| `list?` | `(list? x) → Boolean` | 是否为列表 |

#### 类型判断

| 函数 | 说明 |
|------|------|
| `number?` | 是否为数字 |
| `string?` | 是否为字符串 |
| `symbol?` | 是否为符号 |
| `boolean?` | 是否为布尔值 |
| `procedure?` | 是否为函数 |
| `keyword?` | 是否为关键字 |
| `graphic-object?` | 是否为图形对象 |
| `transform?` / `matrix?` | 是否为变换矩阵 |

#### 字符串操作

| 函数 | 签名 | 说明 |
|------|------|------|
| `string-append` | `(string-append s ...) → String` | 字符串拼接 |
| `string-length` | `(string-length s) → Number` | 字符串长度 |
| `substring` | `(substring s start end) → String` | 子字符串 |
| `string-ref` | `(string-ref s i) → String` | 取第 i 个字符 |
| `number->string` | `(number->string n) → String` | 数字转字符串 |
| `string->number` | `(string->number s) → Number` | 字符串转数字 |
| `format` | `(format fmt . args) → String` | 格式化字符串（`~a` 占位符） |

#### IO 与调试

| 函数 | 签名 | 说明 |
|------|------|------|
| `print` | `(print expr ...) → Nil` | 打印到标准输出 |
| `println` | `(println expr ...) → Nil` | 打印并换行 |
| `error` | `(error msg) → ⊥` | 抛出运行时错误 |
| `assert` | `(assert cond [msg]) → Nil` | 断言，失败时抛出 `AssertionError` |
| `typeof` | `(typeof expr) → Symbol` | 返回值的类型标签 |

### 2.5 模块系统

#### 2.5.1 模块概念

每个 `.pml` 文件构成一个模块。模块拥有独立的顶层作用域，其中的定义默认不对外可见。模块通过 `provide` 显式导出符号，通过 `import` 引入其他模块。

#### 2.5.2 导入与导出

**导出**：

```pml
(provide clamp lerp normalize)
```

`provide` 声明当前模块对外暴露的符号列表。未被 `provide` 声明的符号仅在模块内部可见。

**导入**：

```pml
(import "lib/math.pml" as math)
(define x (math/clamp 5 0 10))
```

`import` 加载指定文件并执行其顶层代码，返回一个模块对象。通过 `prefix/symbol` 语法访问导出符号。

`as` 子句为可选，省略时模块名默认为文件名（不含扩展名）：

```pml
(import "lib/math.pml")    ; 前缀为 math
(math/clamp 5 0 10)
```

#### 2.5.3 模块加载规则

路径解析遵循以下优先级：

1. 相对于当前文件所在目录。
2. 若设置了 `PML_PATH` 环境变量，则在该路径列表中搜索（以冒号或分号分隔）。
3. 解释器自带的标准库目录。

加载约束：

- 循环依赖检测：若 A 导入 B，且 B 直接或间接导入 A，抛出 `CircularImportError`。
- 幂等加载：同一模块在同一运行时中只执行一次，后续 `import` 返回缓存的模块对象。
- 加载顺序：按 `import` 语句的出现顺序依次加载。

#### 2.5.4 模块内部实现

模块对象在 Python 层是一个封装了独立 `Environment` 的 `Module` 实例：

```python
class Module:
    name: str                   # 模块名（用于前缀访问）
    env: Environment            # 模块的顶层环境
    exports: set[str]           # provide 声明的符号集合
    
    def get(self, symbol: str) -> Any:
        """获取导出符号的值，非导出符号抛出 AccessError"""
        if symbol not in self.exports:
            raise AccessError(f"'{symbol}' is not exported from module '{self.name}'")
        return self.env.lookup(symbol)
```

### 2.6 宏系统

#### 2.6.1 宏定义

```pml
(defmacro <name> (<params>) <body>)
```

宏在编译阶段（求值前）展开。宏的参数是**未求值的 S-表达式**（原始语法树），宏体的返回值被**替换**到宏调用的位置，然后再进行求值。

#### 2.6.2 模板语法

宏体通常使用 quasiquote（反引号 `` ` ``）来构造返回的 S-表达式，使用 unquote（逗号 `,`）插入参数值，使用 unquote-splicing（`,@`）展开列表：

```pml
(defmacro swap (a b)
  `(let ((tmp ,a))
     (set! ,a ,b)
     (set! ,b tmp)))

; 使用
(define x 1)
(define y 2)
(swap x y)
; 展开为:
; (let ((tmp x))
;   (set! x y)
;   (set! y tmp))
```

#### 2.6.3 卫生性问题

PML 初期采用非卫生宏。宏作者应使用 `gensym` 生成唯一符号，避免变量捕获问题：

```pml
(defmacro repeat (n . body)
  (let ((i (gensym "repeat-i")))
    `(do ((,i 0 (+ ,i 1)))
         ((>= ,i ,n))
         ,@body)))
```

`gensym` 每次调用返回一个全局唯一的符号（如 `repeat-i_42`），确保不会与用户代码中的变量名冲突。

#### 2.6.4 宏展开时机

宏展开发生在求值之前，按以下规则进行：

1. 解释器遍历顶层表达式。
2. 若表达式的第一个元素是已定义的宏名，则调用宏展开函数，用展开结果替换原表达式。
3. 展开结果可能包含新的宏调用——解释器递归展开，直到不再有宏调用。
4. 递归展开设置最大深度（默认 256），超出则抛出 `MacroExpansionDepthError`。


---

## 第三部分：内置系统

### 3.1 图形图元

所有图元函数返回一个 `GraphicObject`，它是可组合、可变换、可动画的图形实体。

#### 3.1.1 图元列表

| 函数 | 位置参数 | 关键字参数 | 说明 |
|------|---------|-----------|------|
| `(circle x y r)` | 圆心坐标、半径 | `:fill` `:stroke` `:stroke-width` `:transform` | 圆 |
| `(rect x y w h)` | 左上角坐标、宽高 | 同上 | 矩形 |
| `(ellipse cx cy rx ry)` | 中心坐标、x/y 半径 | 同上 | 椭圆 |
| `(line x1 y1 x2 y2)` | 起点、终点 | `:stroke` `:stroke-width` `:transform` | 线段 |
| `(polygon points)` | 点列表 `((x1 y1) (x2 y2) ...)` | `:fill` `:stroke` `:stroke-width` `:transform` | 多边形 |
| `(path d-string)` | SVG path 数据字符串 | `:fill` `:stroke` `:stroke-width` `:transform` | SVG 路径 |
| `(text str x y)` | 文本内容、左下角坐标 | `:font-family` `:font-size` `:fill` `:transform` | 文本 |
| `(image src x y [w h])` | 图片路径、位置、可选宽高 | `:transform` | 位图 |

#### 3.1.2 GraphicObject 数据模型

每个图形对象在内部包含以下属性：

```python
@dataclass
class GraphicObject:
    shape_type: str              # "circle", "rect", "line", ...
    params: dict                 # 图元特定参数
    fill: Optional[str]          # 填充颜色
    stroke: Optional[str]        # 描边颜色
    stroke_width: float          # 描边宽度，默认 1.0
    transform: AffineTransform   # 局部变换矩阵
    children: list               # 子图形（仅 group 使用）
    metadata: dict               # 动画绑定、物理属性等扩展信息
```

图形对象是**不可变**的——任何修改操作（如设置 `:transform`）都返回一个新的对象。这保证了动画系统在时间轴上可以安全地插值而不产生副作用。

#### 3.1.3 颜色值

支持以下颜色格式：

| 格式 | 示例 | 说明 |
|------|------|------|
| 十六进制 | `"#FF0000"`, `"#f00"` | RGB / RGBA |
| 命名颜色 | `"red"`, `"blue"`, `"transparent"` | 预定义颜色表 |
| RGB 函数 | `"rgb(255,0,0)"` | 0-255 范围 |
| RGBA 函数 | `"rgba(255,0,0,0.5)"` | 带透明度 |

`"transparent"` 等价于无填充/无描边。

#### 3.1.4 组合

```pml
(group <graphic-expr> ...)
```

`group` 创建一个包含多个子图形的容器。绘制时按参数顺序从前往后叠加（后绘制的在上层）。`group` 本身也是一个 `GraphicObject`，可以接受 `:transform` 关键字，其变换会级联应用到所有子图形。

```pml
(define face
  (group
    (circle 50 50 40 :fill "yellow")               ; 脸
    (circle 35 40 5 :fill "black")                  ; 左眼
    (circle 65 40 5 :fill "black")                  ; 右眼
    (path "M 35 65 Q 50 80 65 65" :stroke "black") ; 嘴巴
    :transform (translate 200 200)))
```

### 3.2 画布与渲染

#### `(canvas <width> <height> [:bg <color>])`

设置画布尺寸。必须在 `render` 之前调用。背景色可选，默认为白色 `"#FFFFFF"`。

```pml
(canvas 1920 1080 :bg "#f0f0f0")
```

#### `(render <filename> [:format <symbol>] [:fps <number>] [:duration <number>])`

将当前画布上的所有图形输出到文件。

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `filename` | String | 必填 | 输出文件路径 |
| `:format` | Symbol | 从扩展名推断 | `'png`, `'jpg`, `'gif`, `'mp4`, `'svg` |
| `:fps` | Number | 30 | 帧率（仅动画有效） |
| `:duration` | Number | 由动画自动计算 | 总时长（秒） |

### 3.3 变换系统

#### 3.3.1 矩阵表示

PML 使用 6 元素向量 `(a b c d e f)` 表示 2D 仿射变换矩阵：

```
| a  c  e |     x' = a*x + c*y + e
| b  d  f |     y' = b*x + d*y + f
| 0  0  1 |
```

#### 3.3.2 矩阵构造

| 函数 | 签名 | 生成的矩阵 |
|------|------|-----------|
| `(translate dx dy)` | 平移向量 | `(1 0 0 1 dx dy)` |
| `(rotate angle)` | 旋转角度（弧度） | `(cos θ, sin θ, -sin θ, cos θ, 0 0)` |
| `(scale sx sy)` | 缩放因子 | `(sx 0 0 sy 0 0)` |
| `(shear shx shy)` | 错切因子 | `(1 shy shx 1 0 0)` |
| `(transform a b c d e f)` | 直接指定 | `(a b c d e f)` |
| `(identity-matrix)` | 单位矩阵 | `(1 0 0 1 0 0)` |

#### 3.3.3 矩阵组合

```pml
(compose m1 m2)   ; 返回 m1 ∘ m2：先应用 m2，再应用 m1
```

组合顺序的重要性：变换矩阵的乘法不满足交换律。`compose` 遵循数学约定——`(compose (translate 50 0) (rotate 0.785))` 意味着先旋转再平移。

```pml
; 先缩放到 2 倍，再旋转 45 度，最后平移到 (100, 200)
(define m (compose (translate 100 200)
                   (compose (rotate 0.785)
                            (scale 2 2))))
(rect 0 0 50 50 :transform m)
```

#### 3.3.4 矩阵运算

| 函数 | 说明 |
|------|------|
| `(matrix-inverse m)` | 返回逆矩阵 |
| `(matrix-apply m x y)` | 将矩阵应用于点 `(x, y)`，返回 `(x' y')` |
| `(matrix? obj)` | 判断是否为矩阵 |

### 3.4 动画系统

#### 3.4.1 属性动画

```pml
(animate <target> <property> <from> <to> <duration>
         [:ease <symbol>] [:repeat <integer-or-symbol>] [:delay <number>])
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `target` | GraphicObject / 可变引用 | 动画目标对象 |
| `property` | Symbol | 属性名，如 `'x`, `'y`, `'fill`, `'transform`, `'r` |
| `from` | Any | 起始值 |
| `to` | Any | 结束值 |
| `duration` | Number | 持续时间（秒） |
| `:ease` | Symbol | 缓动函数，默认 `'linear` |
| `:repeat` | Number / Symbol | 重复次数，`'infinite` 表示无限循环 |
| `:delay` | Number | 延迟开始时间（秒），默认 0 |

缓动函数列表：

| 符号 | 曲线 |
|------|------|
| `'linear` | 线性 |
| `'quad-in` / `'quad-out` / `'quad-in-out` | 二次方 |
| `'cubic-in` / `'cubic-out` / `'cubic-in-out` | 三次方 |
| `'sin-in` / `'sin-out` / `'sin-in-out` | 正弦 |
| `'bounce` | 弹跳 |
| `'elastic` | 弹性 |

#### 3.4.2 颜色插值

当动画属性为 `:fill` 或 `:stroke` 且值为颜色字符串时，解释器自动在 RGB 空间进行线性插值。对于需要更精确颜色过渡的场景，可在标准库 `color.pml` 中使用 HSV 或 HSL 空间的插值函数。

#### 3.4.3 组合动画

```pml
(parallel <anim> ...)    ; 所有动画同时开始播放
(sequence <anim> ...)    ; 动画依次播放，前一个结束后开始下一个
```

`parallel` 的总时长等于最长子动画的时长。`sequence` 的总时长等于所有子动画时长之和。两者均返回一个 `Animation` 对象，可以进一步组合或控制。

#### 3.4.4 动画控制

| 函数 | 说明 |
|------|------|
| `(play <anim>)` | 开始或恢复播放 |
| `(stop <anim>)` | 停止并重置到初始状态 |
| `(pause <anim>)` | 暂停播放 |
| `(seek <anim> <time>)` | 跳转到指定时间（秒） |
| `(on-finished <anim> <callback>)` | 注册播放完成的回调函数 |
| `(animation-state <anim>)` | 返回当前状态：`'playing`、`'paused`、`'stopped`、`'finished` |

#### 3.4.5 每帧钩子

```pml
(every-frame <thunk>)
```

注册一个无参函数，在每帧渲染前调用。典型用途是在渲染前执行 IK 求解，确保骨骼位置在绘制时是最新的：

```pml
(every-frame (lambda ()
  (ik-solve my-arm 'hand mouse-x mouse-y)))
```

### 3.5 骨骼与逆向动力学

#### 3.5.1 骨骼模板

```pml
(defskeleton <name> (<root-x> <root-y>)
  (joint <joint-name> :pos (<dx> <dy>) :length <len> :angle <init-angle>
         [:min <min-angle>] [:max <max-angle>])
  ...)
```

`defskeleton` 定义一个骨骼模板（`SkeletonTemplate`），描述关节的拓扑结构、初始姿态和活动范围约束。

| 参数 | 说明 |
|------|------|
| `name` | 骨骼模板名称（符号） |
| `root-x`, `root-y` | 骨骼原点的参数名（实例化时提供具体值） |
| `joint-name` | 关节名称（符号） |
| `:pos` | 相对于父关节的偏移量 |
| `:length` | 骨骼段长度 |
| `:angle` | 初始角度（弧度，相对于父骨骼方向） |
| `:min`, `:max` | 关节活动范围约束（弧度），可选 |

示例——定义一条三关节手臂：

```pml
(defskeleton arm (x y)
  (joint shoulder :pos (0 0)  :length 30 :angle 0   :min -1.5 :max 1.5)
  (joint elbow    :pos (30 0) :length 25 :angle 0.3 :min 0    :max 2.0)
  (joint wrist    :pos (25 0) :length 10 :angle 0))
```

#### 3.5.2 骨骼实例化

```pml
(instantiate-skeleton <skeleton-template> (<root-x> <root-y>) [:name <string>])
```

从模板创建骨骼实例。每个实例拥有独立的关节角度状态，互不影响。

```pml
(define left-arm  (instantiate-skeleton arm (200 300) :name "left"))
(define right-arm (instantiate-skeleton arm (400 300) :name "right"))
```

#### 3.5.3 IK 求解

```pml
(ik-solve <skeleton-instance> <end-effector-name> <target-x> <target-y>
          [:method <symbol>] [:iterations <number>] [:tolerance <number>])
```

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `skeleton-instance` | SkeletonInstance | 必填 | 骨骼实例 |
| `end-effector-name` | Symbol | 必填 | 末端效应器关节名 |
| `target-x`, `target-y` | Number | 必填 | 目标位置 |
| `:method` | Symbol | `'fabrik` | 求解算法：`'fabrik` 或 `'ccd` |
| `:iterations` | Number | 10 | 最大迭代次数 |
| `:tolerance` | Number | 0.01 | 收敛容忍度（像素） |

IK 求解器直接修改骨骼实例中各关节的角度值。若在最大迭代次数内未收敛到容忍度内，抛出 `IKNoSolutionError`（严格模式）或返回 `#f`（宽容模式）。

**FABRIK 算法**（Forward And Backward Reaching Inverse Kinematics）：

FABRIK 是一种基于迭代的几何算法。每一轮迭代分为两个阶段：
1. **后向阶段**（Backward）：从末端效应器开始，向根关节逐段调整，使末端对齐目标点。
2. **前向阶段**（Forward）：从根关节开始，向末端逐段恢复，保持根节点固定。

FABRIK 的优势在于计算简单（无需雅可比矩阵），收敛快，且自然支持关节约束。

**CCD 算法**（Cyclic Coordinate Descent）：

CCD 从末端关节向根关节遍历，逐关节旋转使末端更接近目标。实现更简单，但在多关节链中可能产生不自然的姿态。

#### 3.5.4 皮肤绑定

```pml
(bind-skin <graphic-object> <skeleton-instance> <joint-name> ...)
```

将图形对象绑定到指定关节。绑定后，图形的位置和形态会随关节的旋转自动变换。支持多关节绑定——图形会根据关节链的层级关系自动变形。

```pml
(define upper-arm-shape (rect 0 -5 30 10 :fill "gray"))
(bind-skin upper-arm-shape left-arm shoulder)
```

#### 3.5.5 关节查询

```pml
(joint-position <skeleton-instance> <joint-name>)
; 返回 (x y) 全局坐标
```

返回指定关节在世界坐标系中的位置，常用于绘制特效（如关节标记）或施加外部约束。


---

## 第四部分：Sprite 生成架构

本部分描述 PML 面向 sprite 资产生成的专用架构。它在通用图形系统（第三部分）之上构建了一层语义组件层，是 LLM 智能体的主要工作界面。

### 4.1 分层架构总览

PML 的 sprite 生成能力建立在四层架构之上：

```
┌───────────────────────────────────────────────────┐
│              LLM 智能体（外部）                     │
│         生成 PML 代码，调用语义组件 API              │
└──────────────────────┬────────────────────────────┘
                       │ PML 源代码
                       ▼
┌───────────────────────────────────────────────────┐
│         第四层：组装 API (Assembly API)              │
│  character / item / ui-widget / scene-tile         │
│  高层组件装配函数，接收语义参数                       │
└──────────────────────┬────────────────────────────┘
                       │ GraphicObject 组合
                       ▼
┌───────────────────────────────────────────────────┐
│         第三层：语义组件库 (Semantic Components)      │
│  anime-eyes / hair / body / outfit / weapon ...    │
│  参数化的领域组件，每个组件内部由底层图元构成          │
└──────────────────────┬────────────────────────────┘
                       │ GraphicObject (基本图元)
                       ▼
┌───────────────────────────────────────────────────┐
│         第二层：图形图元 (Graphics Primitives)        │
│  circle / rect / path / line / polygon / group     │
│  通用几何图元 + 变换 + 颜色                          │
└──────────────────────┬────────────────────────────┘
                       │ 绘图指令
                       ▼
┌───────────────────────────────────────────────────┐
│         第一层：渲染后端 (Render Backend)             │
│  Pillow / Cairo / SVG / 自定义后端                  │
│  将绘图指令光栅化或矢量化为输出文件                   │
└───────────────────────────────────────────────────┘
```

LLM 智能体主要在**第四层**（组装 API）和**第三层**（语义组件库）工作。当需要精细调整时，可以下探到第二层图元。

### 4.2 风格系统 (Style System)

风格系统解决的核心问题是：当一个角色由多个独立组件（头发、眼睛、身体、服装）组合而成时，如何保证它们在视觉上风格一致？

#### 4.2.1 风格描述符 (Style Descriptor)

风格描述符是一个数据结构，定义了影响所有组件外观的全局视觉参数：

```pml
(define-style <name>
  :outline-width <number>        ; 描边宽度，默认 2
  :outline-color <color>         ; 描边颜色，默认 "#000"
  :outline-style <symbol>        ; 'solid | 'dashed | 'none
  :shading <symbol>              ; 'flat | 'cel | 'gradient | 'pixel
  :palette <palette-ref>         ; 调色板引用
  :shadow <boolean-or-color>     ; 是否启用阴影 / 阴影颜色
  :highlight <boolean-or-color>  ; 是否启用高光 / 高光颜色
  :pixel-size <number>           ; 像素风专用：逻辑像素大小
  :anti-alias <boolean>          ; 是否抗锯齿
  :corner-radius <number>)       ; 圆角半径（UI 组件）
```

#### 4.2.2 预定义风格模板

解释器内置以下风格模板，用户也可直接定义新风格：

| 风格名 | 特征 | 典型用途 |
|--------|------|---------|
| `'cel` | 粗描边 + 平面色块 + 赛璐珞阴影 | 日系动漫角色 |
| `'pixel` | 无抗锯齿 + 离散调色板 + 逻辑像素网格 | 复古像素游戏 |
| `'flat` | 细描边 + 纯色填充 + 无阴影 | 现代 UI / 图标 |
| `'soft` | 无描边 + 渐变填充 + 柔光阴影 | 柔和插画风 |
| `'chibi` | 粗描边 + 大头小身比例 + 高饱和 | Q 版角色 |
| `'outline` | 纯线稿 + 无填充 | 线稿/草图 |

使用方式：

```pml
(use-style 'cel)  ; 全局应用赛璐珞风格
; 或
(define hero (character ... :style 'cel))  ; 仅对特定角色应用
```

#### 4.2.3 调色板 (Palette)

调色板是一组命名颜色的集合，风格系统通过调色板约束组件的配色，确保视觉一致性：

```pml
(define-palette "dark-hero"
  :primary   "#2c3e50"
  :secondary "#3498db"
  :accent    "#e74c3c"
  :skin      "#fce4c8"
  :hair      "#2c2c2c"
  :outline   "#1a1a2e")
```

组件可以引用调色板中的命名颜色：

```pml
(character
  :palette "dark-hero"
  :hair (hair :color (palette-ref "hair"))     ; => "#2c2c2c"
  :eyes (anime-eyes :color (palette-ref "secondary")))
```

#### 4.2.4 风格传播机制

当在 `character` 或 `scene` 级别指定 `:style` 时，风格描述符会**向下传播**到所有子组件。子组件可以通过显式参数覆盖特定属性：

```pml
(character
  :style 'cel                    ; 所有子组件继承 cel 风格
  :body (body :skin "#fce4c8")
  :eyes (anime-eyes 
          :style 'soft           ; 眼睛单独使用 soft 风格（覆盖）
          :color "#4a90d9"))
```

传播规则：
1. 父级 `:style` 作为默认值传递给所有子组件。
2. 子组件的显式参数（如 `:outline-width 3`）覆盖风格描述符中的对应值。
3. 子组件的 `:style` 参数完全替换从父级继承的风格（而非合并）。

### 4.3 语义组件库 (Semantic Component Library)

语义组件库是 PML sprite 生成能力的核心。每个组件都是一个**参数化生成器**——接收语义级别的参数，内部通过底层图元组合来渲染。

#### 4.3.1 组件设计原则

**有限且合法的参数空间**。每个组件的参数都有明确的取值范围或枚举集。解释器对越界参数进行 clamp 或 fallback 到最近合法值，而非报错。这保证 LLM 在合法空间内的任意组合都能产出可用的 sprite：

```pml
; :style 是枚举集，只能取预定义值
(anime-eyes :style "shoujo")   ; ✓ 合法
(anime-eyes :style "xyz")      ; → fallback 到默认风格，打印警告

; :length 是连续值，自动 clamp 到 [10, 200]
(hair :length 500)             ; → clamp 到 200
```

**组合而非继承**。组件之间没有继承关系，而是通过组合来构建复杂对象。每个组件都是独立的、可替换的：

```pml
; 可以自由混搭不同风格的组件
(character
  :head (head :shape "round")
  :eyes (anime-eyes :style "shoujo")     ; 可以换成 "sharp"、"round" 等
  :hair (hair :style "ponytail")         ; 可以换成 "spiky"、"bob" 等
  :outfit (outfit :top "hoodie"))
```

**默认即合理**。每个参数都有精心选择的默认值，使得 `(character)` 零参数调用也能生成一个完整的、可用的角色 sprite。

#### 4.3.2 角色组件 (Character Components)

角色组件库是语义层中最丰富的部分。以下是核心组件及其参数：

**`(character ...)` — 角色组装器**

顶层组装函数，将各部件装配为完整角色：

```pml
(character
  :body <body-obj>              ; 身体组件
  :head <head-obj>              ; 头部组件（含面部特征）
  :hair <hair-obj>              ; 发型组件
  :outfit <outfit-obj>          ; 服装组件
  :accessory <accessory-obj>    ; 配饰（可选）
  :weapon <item-obj>            ; 武器/道具（可选）
  :pose <symbol>                ; 姿态：'standing | 'walking | 'running | 'sitting | 'action
  :direction <symbol>           ; 朝向：'front | 'back | 'left | 'right | 'front-left ...
  :expression <symbol>          ; 表情：'neutral | 'happy | 'angry | 'sad | 'surprised ...
  :style <style-ref>            ; 风格
  :palette <palette-ref>        ; 调色板
  :scale <number>)              ; 全局缩放，默认 1.0
```

**`(body ...)` — 身体**

```pml
(body
  :height <number>              ; 身高（像素），默认 160
  :build <symbol>               ; 体型：'slim | 'average | 'muscular | 'chubby
  :skin <color>                 ; 肤色
  :proportions <symbol>)        ; 比例：'realistic | 'anime | 'chibi
```

**`(head ...)` — 头部**

```pml
(head
  :shape <symbol>               ; 脸型：'oval | 'round | 'square | 'heart | 'angular
  :skin <color>                 ; 肤色（覆盖 body 设置）
  :ears <symbol>)               ; 耳朵：'normal | 'pointed | 'animal | 'none
```

**`(anime-eyes ...)` — 动漫眼睛**

```pml
(anime-eyes
  :style <symbol>               ; 眼型：'shoujo | 'shounen | 'round | 'sharp | 'sleepy | 'closed
  :color <color>                ; 虹膜颜色
  :size <number>                ; 大小比例，默认 1.0
  :spacing <number>             ; 间距比例，默认 1.0
  :highlight <boolean>          ; 是否绘制高光，默认 #t
  :lashes <symbol>)             ; 睫毛：'none | 'short | 'long
```

**`(mouth ...)` — 嘴部**

```pml
(mouth
  :style <symbol>               ; 嘴型：'neutral | 'smile | 'frown | 'open | 'cat | 'fang
  :size <number>)               ; 大小比例，默认 1.0
```

**`(hair ...)` — 发型**

```pml
(hair
  :style <symbol>               ; 发型：'short | 'medium | 'long | 'ponytail | 'twintails
                                ;        'spiky | 'bob | 'braid | 'bun | 'mohawk | 'bald
  :color <color>                ; 发色
  :length <number>              ; 长度（像素），根据 style 自动推断
  :bangs <symbol>               ; 刘海：'straight | 'side | 'curtain | 'none
  :accessory <symbol>)          ; 发饰：'none | 'ribbon | 'clip | 'band | 'flower
```

**`(outfit ...)` — 服装**

```pml
(outfit
  :top <symbol>                 ; 上衣：'t-shirt | 'jacket | 'hoodie | 'dress | 'armor
                                ;        'robe | 'suit | 'tank | 'sailor
  :bottom <symbol>              ; 下装：'pants | 'skirt | 'shorts | 'long-skirt | 'armor
  :shoes <symbol>               ; 鞋子：'boots | 'sneakers | 'sandals | 'heels | 'none
  :color-top <color>            ; 上衣颜色
  :color-bottom <color>         ; 下装颜色
  :detail <symbol>)             ; 细节：'plain | 'striped | 'pattern | 'badge
```

#### 4.3.3 物品组件 (Item Components)

用于生成游戏道具、武器、消耗品等 sprite：

**`(weapon ...)` — 武器**

```pml
(weapon
  :type <symbol>                ; 'sword | 'axe | 'bow | 'staff | 'dagger | 'spear | 'gun
  :size <symbol>                ; 'small | 'medium | 'large | 'legendary
  :material <symbol>            ; 'iron | 'steel | 'gold | 'crystal | 'wood | 'bone
  :element <symbol>             ; 'none | 'fire | 'ice | 'lightning | 'holy | 'dark
  :ornament <symbol>)           ; 'plain | 'gem | 'rune | 'engraved
```

**`(potion ...)` — 药水/消耗品**

```pml
(potion
  :type <symbol>                ; 'health | 'mana | 'buff | 'poison | 'bomb
  :container <symbol>           ; 'bottle | 'flask | 'vial | 'jar
  :color <color>                ; 液体颜色
  :size <symbol>)               ; 'small | 'medium | 'large
```

**`(chest ...)` — 宝箱/容器**

```pml
(chest
  :type <symbol>                ; 'wooden | 'iron | 'gold | 'crystal
  :state <symbol>               ; 'closed | 'open | 'locked
  :size <symbol>)               ; 'small | 'medium | 'large
```

**`(generic-item ...)` — 通用物品**

用于自定义物品，提供更灵活的图元级控制：

```pml
(generic-item
  :name <string>                ; 物品名称（用于元数据）
  :base-shape <symbol>          ; 'circle | 'rect | 'diamond | 'custom
  :color <color>
  :detail <graphic-object>      ; 叠加的细节图元
  :outline <boolean>)
```

#### 4.3.4 UI 组件 (UI Widgets)

用于生成游戏 UI 元素 sprite：

**`(button ...)` — 按钮**

```pml
(button
  :label <string>               ; 按钮文字
  :width <number>               ; 宽度（像素）
  :height <number>              ; 高度（像素）
  :style <symbol>               ; 'rounded | 'sharp | 'pixel | 'ornate
  :state <symbol>               ; 'normal | 'hover | 'pressed | 'disabled
  :color <color>                ; 主色
  :text-color <color>)          ; 文字颜色
```

**`(panel ...)` — 面板/对话框**

```pml
(panel
  :width <number>
  :height <number>
  :style <symbol>               ; 'simple | 'ornate | 'pixel | 'glass
  :title <string>               ; 标题（可选）
  :color <color>
  :border-width <number>)
```

**`(health-bar ...)` — 血条/进度条**

```pml
(health-bar
  :value <number>               ; 当前值（0-1）
  :width <number>
  :height <number>
  :color <color>                ; 填充颜色
  :bg-color <color>             ; 背景颜色
  :style <symbol>)              ; 'flat | 'segmented | 'gradient
```

**`(icon ...)` — 图标**

```pml
(icon
  :type <symbol>                ; 'heart | 'star | 'coin | 'gem | 'shield | 'skull | ...
  :size <number>
  :color <color>
  :style <symbol>)              ; 'flat | 'pixel | 'detailed
```

#### 4.3.5 场景元素 (Scene Tiles)

用于生成 tilemap 瓦片和场景装饰物：

**`(tile ...)` — 地面/墙壁瓦片**

```pml
(tile
  :type <symbol>                ; 'grass | 'stone | 'wood | 'sand | 'water | 'snow | 'brick
  :size <number>                ; 瓦片尺寸（像素），默认 32
  :variant <number>             ; 变体编号（0-3），用于打破重复感
  :edge <symbol>)               ; 'none | 'top | 'bottom | 'left | 'right | 'corner-*
```

**`(decoration ...)` — 场景装饰**

```pml
(decoration
  :type <symbol>                ; 'tree | 'bush | 'rock | 'flower | 'mushroom | 'crate
                                ; 'barrel | 'torch | 'sign | 'fence | 'lamp
  :size <symbol>                ; 'small | 'medium | 'large
  :season <symbol>              ; 'spring | 'summer | 'autumn | 'winter
  :variant <number>)            ; 变体编号
```

**`(background ...)` — 背景层**

```pml
(background
  :type <symbol>                ; 'sky | 'forest | 'dungeon | 'town | 'ocean | 'mountain
  :time <symbol>                ; 'day | 'dusk | 'night | 'dawn
  :weather <symbol>             ; 'clear | 'cloudy | 'rain | 'snow | 'fog
  :width <number>               ; 背景宽度
  :height <number>              ; 背景高度
  :parallax <number>)           ; 视差因子（用于多层背景），默认 1.0
```

### 4.4 Sprite 输出规范

PML 的输出不是一张普通的图片——它是游戏引擎可直接消费的 sprite 资产。这需要严格的输出规格控制。

#### 4.4.1 画框与裁切 (Framing)

每个 sprite 都应输出到固定尺寸的画框中，确保在游戏引擎中对齐和动画帧一致性：

```pml
(sprite-canvas <width> <height>
  :bg <color>                   ; 背景色，默认 "transparent"
  :anchor <symbol-or-point>     ; 锚点位置
  :padding <number>)            ; 内边距（像素），默认 0
```

锚点定义 sprite 在游戏世界中的"脚底"或"中心"参考点：

| 锚点值 | 含义 |
|--------|------|
| `'center` | 画框中心 `(w/2, h/2)` |
| `'center-bottom` | 底部中心 `(w/2, h)` — 角色 sprite 的默认锚点 |
| `'top-left` | 左上角 `(0, 0)` |
| `'top-right` | 右上角 `(w, 0)` |
| `'bottom-left` | 左下角 `(0, h)` |
| `'bottom-right` | 右下角 `(w, h)` |
| `(x y)` | 自定义坐标 |

锚点信息会被嵌入输出的元数据中（PNG 的 tEXt 块或单独的 JSON 文件），供游戏引擎读取。

#### 4.4.2 Sprite Sheet 输出

Sprite sheet 是将多个帧或变体排列在一张大图上的标准格式：

```pml
(render "hero_walk.png"
  :format 'png
  :sheet (4 8)                  ; 4 列 × 8 行
  :frame-size (64 64)           ; 每帧尺寸
  :frames (list walk-frame-1 walk-frame-2 ...))
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `:sheet` | `(cols rows)` | sheet 的列数和行数 |
| `:frame-size` | `(w h)` | 每帧的像素尺寸 |
| `:frames` | List | 帧内容列表（GraphicObject 列表） |
| `:spacing` | Number | 帧间距（像素），默认 0 |
| `:anchor` | Symbol | 每帧共享的锚点 |

输出时，解释器自动将帧列表按行优先顺序排列到 sheet 中，并生成配套的元数据文件（JSON）：

```json
{
  "image": "hero_walk.png",
  "frameSize": [64, 64],
  "columns": 4,
  "rows": 8,
  "anchor": [32, 64],
  "frames": [
    {"name": "walk_0", "rect": [0, 0, 64, 64]},
    {"name": "walk_1", "rect": [64, 0, 64, 64]}
  ]
}
```

#### 4.4.3 多分辨率输出

解释器支持从单一描述生成多分辨率 sprite：

```pml
(render-set "hero"
  :content hero-graphic
  :scales '(1 2 4)              ; 1x, 2x, 4x
  :base-size (64 64)            ; 基准尺寸
  :format 'png)
; 输出: hero@1x.png (64×64), hero@2x.png (128×128), hero@4x.png (256×256)
```

缩放时，根据当前风格自动选择插值算法：
- `'pixel` 风格：最近邻插值（保持像素锐利）
- 其他风格：双线性或双三次插值（平滑过渡）

#### 4.4.4 输出元数据

每次 `render` 输出时，解释器可选生成一个 `.meta.json` 文件，包含：

```json
{
  "source": "hero.pml",
  "style": "cel",
  "palette": "dark-hero",
  "anchor": [32, 64],
  "frameSize": [64, 64],
  "components": ["body", "head", "hair", "outfit", "eyes"],
  "generatedAt": "2026-06-15T12:00:00+08:00",
  "pmlVersion": "0.1.0"
}
```

这使得 sprite 可以被后续工具链（如打包器、游戏引擎导入器）正确识别和处理。

### 4.5 语义组件的内部实现

#### 4.5.1 组件注册

语义组件在 Python 层是**组件工厂函数**，通过注册机制绑定到 PML 符号：

```python
@sprite_component("anime-eyes")
def create_anime_eyes(args: list, kwargs: dict, style: StyleDescriptor) -> GraphicObject:
    """创建动漫风格的眼睛组件。
    
    内部逻辑：
    1. 从 kwargs 读取 :style, :color, :size 等参数
    2. 根据 :style 选择预定义的 path 数据
    3. 应用 style descriptor 的描边/阴影/高光设置
    4. 返回组合后的 GraphicObject
    """
    eye_style = kwargs.get("style", "shoujo")
    color = kwargs.get("color", "#4a90d9")
    size = kwargs.get("size", 1.0)
    
    # 从预定义模板中获取 path 数据
    template = EYE_TEMPLATES.get(eye_style, EYE_TEMPLATES["shoujo"])
    
    # 构建 GraphicObject
    left_eye = build_eye(template, color, size, style, mirror=False)
    right_eye = build_eye(template, color, size, style, mirror=True)
    
    return group(left_eye, right_eye, spacing=kwargs.get("spacing", 1.0))
```

#### 4.5.2 组件模板数据

每个语义组件背后有一组**模板数据**（Template Data），定义了该组件不同变体的几何形状。这些数据以 Python 数据结构存储，包含 SVG path 控制点、比例参数等：

```python
EYE_TEMPLATES = {
    "shoujo": {
        "outer_path": "M ...",         # 外轮廓 SVG path
        "iris_path": "M ...",          # 虹膜形状
        "pupil_offset": (0, 2),        # 瞳孔偏移
        "highlight_positions": [(8, 5), (3, 8)],  # 高光位置
        "default_size": (24, 28),      # 默认尺寸
        "aspect_ratio": 0.85,          # 宽高比
    },
    "sharp": {
        "outer_path": "M ...",
        # ...
    },
    # ...
}
```

模板数据是组件库的核心资产。初始版本通过手工定义，后续可以引入参数化生成（如通过少量控制点 + 插值生成曲线）。

#### 4.5.3 组件验证

组件函数在接收参数后、渲染前，执行**参数验证与修正**：

```python
def validate_and_clamp(kwargs: dict, spec: ComponentSpec) -> dict:
    """验证组件参数，对越界值进行 clamp 或 fallback"""
    result = {}
    for param_name, param_spec in spec.params.items():
        value = kwargs.get(param_name, param_spec.default)
        
        if param_spec.kind == "enum":
            if value not in param_spec.allowed:
                log_warning(f"Invalid {param_name} '{value}', "
                           f"falling back to '{param_spec.default}'")
                value = param_spec.default
        
        elif param_spec.kind == "range":
            value = max(param_spec.min_val, min(param_spec.max_val, value))
        
        result[param_name] = value
    return result
```

这种"宽容验证"策略确保 LLM 生成的代码即使参数不完美也能产出可用结果，仅在控制台打印警告。

#### 4.5.4 组件库目录结构

语义组件库的 `.pml` 文件（供 LLM 参考的 API 文档）和 Python 实现文件组织如下：

```
stdlib/sprites/
├── character.pml          ; character 组装器 API 文档 + 示例
├── body.pml               ; body 组件
├── head.pml               ; head 组件
├── eyes.pml               ; anime-eyes 组件
├── mouth.pml              ; mouth 组件
├── hair.pml               ; hair 组件
├── outfit.pml             ; outfit 组件
├── items/
│   ├── weapons.pml        ; weapon 组件
│   ├── potions.pml        ; potion 组件
│   └── containers.pml     ; chest 组件
├── ui/
│   ├── button.pml         ; button 组件
│   ├── panel.pml          ; panel 组件
│   ├── bars.pml           ; health-bar / mana-bar 组件
│   └── icons.pml          ; icon 组件
├── scene/
│   ├── tiles.pml          ; tile 组件
│   ├── decorations.pml    ; decoration 组件
│   └── backgrounds.pml    ; background 组件
├── styles/
│   ├── cel.pml            ; 赛璐珞风格定义
│   ├── pixel.pml          ; 像素风定义
│   ├── flat.pml           ; 扁平风定义
│   └── palettes.pml       ; 预定义调色板集合
└── README.pml             ; 组件库总览与使用指南
```

对应的 Python 实现在核心包内：

```
pml/sprites/
├── __init__.py
├── registry.py            ; 组件注册器
├── validator.py           ; 参数验证
├── character.py           ; character 组装器实现
├── body_parts.py          ; body / head / eyes / mouth 实现
├── hair.py                ; hair 组件实现
├── outfit.py              ; outfit 组件实现
├── items.py               ; weapon / potion / chest 实现
├── ui_widgets.py          ; button / panel / bars / icon 实现
├── scene_elements.py      ; tile / decoration / background 实现
├── style.py               ; 风格描述符与传播
├── palette.py             ; 调色板管理
└── templates/             ; 模板数据（JSON/YAML）
    ├── eyes.json
    ├── hair.json
    ├── outfits.json
    └── ...
```

### 4.6 LLM 接口层

#### 4.6.1 API 模式

除了文件执行和 REPL，PML 解释器还提供一个 **API 接口**，供外部 LLM 智能体调用：

```python
class PMLRuntime:
    def __init__(self, style: str = "cel", output_dir: str = "./output"):
        """初始化 PML 运行时"""
    
    def execute(self, source: str) -> RenderResult:
        """执行 PML 源代码，返回渲染结果"""
    
    def render_sprite(self, source: str, **options) -> SpriteAsset:
        """执行源代码并输出 sprite 资产（含元数据）"""
    
    def validate(self, source: str) -> ValidationResult:
        """仅验证源代码的语法和参数合法性，不渲染"""
    
    def list_components(self, category: str = None) -> list[ComponentInfo]:
        """列出可用的语义组件及其参数规格"""
    
    def preview_params(self, component: str) -> dict:
        """返回指定组件的参数规格（类型、范围、默认值、枚举集）"""
```

#### 4.6.2 组件发现与参数规格

LLM 在生成 PML 代码前，可以查询可用的组件和参数规格。`list_components` 和 `preview_params` 返回结构化的 JSON 数据，便于 LLM 理解 API 边界：

```json
{
  "component": "anime-eyes",
  "category": "character",
  "description": "Anime-style eye component",
  "params": [
    {"name": "style", "type": "enum", "values": ["shoujo", "shounen", "round", "sharp", "sleepy", "closed"], "default": "shoujo"},
    {"name": "color", "type": "color", "default": "#4a90d9"},
    {"name": "size", "type": "range", "min": 0.5, "max": 2.0, "default": 1.0},
    {"name": "spacing", "type": "range", "min": 0.5, "max": 2.0, "default": 1.0},
    {"name": "highlight", "type": "boolean", "default": true},
    {"name": "lashes", "type": "enum", "values": ["none", "short", "long"], "default": "none"}
  ]
}
```

#### 4.6.3 错误反馈给 LLM

当 LLM 生成的 PML 代码出现错误时，错误信息应结构化且包含修复建议，便于 LLM 自我修正：

```json
{
  "error": {
    "type": "InvalidParam",
    "component": "anime-eyes",
    "param": "style",
    "value": "xyz",
    "allowed": ["shoujo", "shounen", "round", "sharp", "sleepy", "closed"],
    "hint": "Use one of the allowed eye styles. 'shoujo' is the default.",
    "line": 5,
    "column": 18
  }
}
```


---

## 第五部分：解释器架构

### 5.1 整体流水线

PML 解释器的执行流水线分为四个阶段：

```
源代码 (.pml)
    │
    ▼
┌──────────┐
│  Lexer   │  词法分析：字符流 → Token 流
└────┬─────┘
     │ Token[]
     ▼
┌──────────┐
│  Parser  │  语法分析：Token 流 → AST（S-表达式树）
└────┬─────┘
     │ Expr (嵌套列表)
     ▼
┌──────────────┐
│  Expander    │  宏展开：展开所有宏调用
└────┬─────────┘
     │ Expr (无宏)
     ▼
┌──────────────┐
│  Evaluator   │  求值：AST → 运行时值 + 副作用
└────┬─────────┘
     │
     ▼
  输出（图形/文件/REPL 打印）
```

### 5.2 项目目录结构

```
pml/
├── pml/                        # 核心解释器包
│   ├── __init__.py
│   ├── lexer.py                # 词法分析器
│   ├── parser.py               # 语法分析器
│   ├── ast_nodes.py            # AST 节点定义
│   ├── evaluator.py            # 求值器（核心）
│   ├── environment.py          # 环境（作用域）系统
│   ├── expander.py             # 宏展开器
│   ├── types.py                # 运行时类型定义
│   ├── errors.py               # 异常类型
│   ├── module_loader.py        # 模块加载器
│   ├── builtins/               # 内置函数
│   │   ├── __init__.py         # 注册所有内置函数
│   │   ├── arithmetic.py       # 算术运算
│   │   ├── comparison.py       # 比较运算
│   │   ├── logic.py            # 逻辑运算
│   │   ├── list_ops.py         # 列表操作
│   │   ├── string_ops.py       # 字符串操作
│   │   ├── io.py               # IO 与调试
│   │   └── type_predicates.py  # 类型判断
│   ├── graphics/               # 图形系统
│   │   ├── __init__.py
│   │   ├── primitives.py       # 图元定义
│   │   ├── graphic_object.py   # GraphicObject 数据类
│   │   ├── canvas.py           # 画布管理
│   │   └── color.py            # 颜色解析与插值
│   ├── transform/              # 变换系统
│   │   ├── __init__.py
│   │   └── affine.py           # 仿射矩阵
│   ├── sprites/                # 语义组件库（Python 实现）
│   │   ├── __init__.py
│   │   ├── registry.py         # 组件注册器
│   │   ├── validator.py        # 参数验证与 clamp
│   │   ├── character.py        # character 组装器
│   │   ├── body_parts.py       # body / head / eyes / mouth
│   │   ├── hair.py             # hair 组件
│   │   ├── outfit.py           # outfit 组件
│   │   ├── items.py            # weapon / potion / chest
│   │   ├── ui_widgets.py       # button / panel / bars / icon
│   │   ├── scene_elements.py   # tile / decoration / background
│   │   ├── style.py            # 风格描述符与传播
│   │   ├── palette.py          # 调色板管理
│   │   ├── sprite_output.py    # sprite 画框/锚点/sheet 输出
│   │   └── templates/          # 组件模板数据（JSON）
│   │       ├── eyes.json
│   │       ├── hair.json
│   │       ├── bodies.json
│   │       ├── outfits.json
│   │       ├── weapons.json
│   │       ├── tiles.json
│   │       └── ui.json
│   ├── animation/              # 动画系统
│   │   ├── __init__.py
│   │   ├── engine.py           # 动画引擎
│   │   ├── easing.py           # 缓动函数
│   │   └── timeline.py         # 时间轴管理
│   ├── skeleton/               # 骨骼与 IK
│   │   ├── __init__.py
│   │   ├── skeleton.py         # 骨骼模板与实例
│   │   ├── ik_fabrik.py        # FABRIK 求解器
│   │   └── ik_ccd.py           # CCD 求解器
│   ├── backend/                # 渲染后端
│   │   ├── __init__.py
│   │   ├── base.py             # 后端抽象基类
│   │   ├── cairo_backend.py    # Cairo 后端（PNG/SVG）
│   │   └── pillow_backend.py   # Pillow 后端（PNG/JPG/GIF）
│   ├── api.py                  # LLM API 接口（PMLRuntime）
│   └── repl.py                 # REPL 交互环境
├── stdlib/                     # 标准库 .pml 文件
│   ├── math.pml
│   ├── color.pml
│   ├── easing.pml
│   ├── shapes.pml
│   └── sprites/                # 语义组件库 API 文档
│       ├── character.pml
│       ├── body.pml
│       ├── eyes.pml
│       ├── hair.pml
│       ├── outfit.pml
│       ├── items/
│       │   ├── weapons.pml
│       │   ├── potions.pml
│       │   └── containers.pml
│       ├── ui/
│       │   ├── button.pml
│       │   ├── panel.pml
│       │   ├── bars.pml
│       │   └── icons.pml
│       ├── scene/
│       │   ├── tiles.pml
│       │   ├── decorations.pml
│       │   └── backgrounds.pml
│       └── styles/
│           ├── cel.pml
│           ├── pixel.pml
│           ├── flat.pml
│           └── palettes.pml
├── examples/                   # 示例程序
│   ├── hello_circle.pml
│   ├── hero_character.pml      # 完整的角色 sprite 生成
│   ├── game_items.pml          # 游戏物品 sprite
│   ├── ui_kit.pml              # UI 控件集
│   ├── tileset.pml             # 瓦片集生成
│   └── ...
├── tests/                      # 测试
│   ├── test_lexer.py
│   ├── test_parser.py
│   ├── test_evaluator.py
│   ├── test_modules.py
│   ├── test_graphics.py
│   ├── test_sprites.py         # 语义组件库测试
│   ├── test_style.py           # 风格系统测试
│   ├── test_sprite_output.py   # sprite 输出测试
│   ├── test_animation.py
│   ├── test_ik.py
│   ├── test_api.py             # LLM API 测试
│   └── fixtures/               # 测试用的 .pml 文件
│       └── ...
├── docs/                       # 文档
│   ├── design.md               # 语言规范（精简版）
│   └── development.md          # 本文档
├── pyproject.toml              # 项目配置
├── README.md
└── LICENSE
```

### 5.3 词法分析器 (Lexer)

文件：`pml/lexer.py`

词法分析器将源文本转化为 Token 流。它采用手写扫描器（非正则引擎），以支持精确的错误定位。

#### Token 类型定义

```python
from enum import Enum, auto
from dataclasses import dataclass

class TokenType(Enum):
    INTEGER = auto()
    FLOAT = auto()
    STRING = auto()
    SYMBOL = auto()
    BOOLEAN = auto()
    KEYWORD = auto()
    LPAREN = auto()       # (
    RPAREN = auto()       # )
    QUOTE = auto()         # '
    QUASIQUOTE = auto()    # `
    UNQUOTE = auto()       # ,
    UNQUOTE_SPLICE = auto() # ,@
    EOF = auto()

@dataclass
class Token:
    type: TokenType
    value: str
    line: int              # 行号（1-indexed）
    column: int            # 列号（1-indexed）
```

#### 扫描算法

```python
class Lexer:
    def __init__(self, source: str, filename: str = "<stdin>"):
        self.source = source
        self.filename = filename
        self.pos = 0
        self.line = 1
        self.column = 1
    
    def tokenize(self) -> list[Token]:
        """将完整源码转为 Token 列表"""
        tokens = []
        while self.pos < len(self.source):
            self.skip_whitespace_and_comments()
            if self.pos >= len(self.source):
                break
            token = self.read_token()
            if token:
                tokens.append(token)
        tokens.append(Token(TokenType.EOF, "", self.line, self.column))
        return tokens
```

关键实现细节：

- **字符串解析**：支持 `\"` 和 `\\` 转义。遇到未终止的字符串时，抛出 `SyntaxError` 并附带起始行号。
- **数字解析**：先尝试整数，遇到小数点则转为浮点。以 `+` 或 `-` 开头时，需检查后续字符是数字还是符号。
- **符号解析**：读取直到遇到空白或特殊字符（括号、引号等）。
- **错误恢复**：遇到非法字符时，记录错误并尝试跳到下一个有效 Token 继续扫描，最终汇总所有词法错误。

### 5.4 语法分析器 (Parser)

文件：`pml/parser.py`

语法分析器将 Token 流转化为嵌套的 Python 列表（即 AST）。由于 PML 的语法本质上是括号化列表，Parser 的实现非常直接。

#### AST 表示

PML 的 AST 不需要复杂的节点类型——它直接复用 Python 的内置数据结构：

```python
# AST 类型定义
Atom = int | float | str | bool | None
Expr = Atom | list[Expr]
```

| PML 构造 | Python 表示 |
|---------|------------|
| 整数 `42` | `42` |
| 浮点数 `3.14` | `3.14` |
| 字符串 `"hello"` | `"hello"` |
| 符号 `circle` | `Symbol("circle")` |
| 布尔 `#t` | `True` |
| 关键字 `:fill` | `Keyword("fill")` |
| 列表 `(+ 1 2)` | `[Symbol("+"), 1, 2]` |

`Symbol` 和 `Keyword` 是轻量的包装类，用于区分普通字符串：

```python
@dataclass(frozen=True)
class Symbol:
    name: str
    def __repr__(self):
        return f"Symbol({self.name})"

@dataclass(frozen=True)
class Keyword:
    name: str  # 不含冒号前缀
    def __repr__(self):
        return f":{self.name}"
```

#### 解析算法

```python
class Parser:
    def __init__(self, tokens: list[Token]):
        self.tokens = tokens
        self.pos = 0
    
    def parse(self) -> list[Expr]:
        """解析整个文件，返回顶层表达式列表"""
        exprs = []
        while not self.at_end():
            exprs.append(self.parse_expr())
        return exprs
    
    def parse_expr(self) -> Expr:
        token = self.current()
        if token.type == TokenType.LPAREN:
            return self.parse_list()
        elif token.type == TokenType.QUOTE:
            self.advance()
            return [Symbol("quote"), self.parse_expr()]
        elif token.type == TokenType.QUASIQUOTE:
            self.advance()
            return [Symbol("quasiquote"), self.parse_expr()]
        elif token.type == TokenType.UNQUOTE_SPLICE:
            self.advance()
            return [Symbol("unquote-splicing"), self.parse_expr()]
        elif token.type == TokenType.UNQUOTE:
            self.advance()
            return [Symbol("unquote"), self.parse_expr()]
        else:
            return self.parse_atom()
```

错误处理：括号不匹配时抛出 `SyntaxError`，附带未匹配括号的位置信息。

### 5.5 环境系统 (Environment)

文件：`pml/environment.py`

环境是符号到值的映射，支持词法作用域嵌套。

```python
class Environment:
    def __init__(self, parent: Optional['Environment'] = None, 
                 bindings: dict[str, Any] = None):
        self.parent = parent
        self.bindings: dict[str, Any] = bindings or {}
    
    def lookup(self, name: str) -> Any:
        """查找符号，从当前环境向外层逐级搜索"""
        if name in self.bindings:
            return self.bindings[name]
        if self.parent:
            return self.parent.lookup(name)
        raise UnboundVariableError(f"Unbound variable: {name}")
    
    def define(self, name: str, value: Any) -> None:
        """在当前环境中定义（或覆盖）符号"""
        self.bindings[name] = value
    
    def set(self, name: str, value: Any) -> None:
        """修改已有绑定的值，沿作用域链搜索"""
        if name in self.bindings:
            self.bindings[name] = value
            return
        if self.parent:
            self.parent.set(name, value)
            return
        raise UnboundVariableError(f"Cannot set unbound variable: {name}")
    
    def extend(self, names: list[str], values: list[Any]) -> 'Environment':
        """创建子环境，绑定参数列表"""
        new_env = Environment(parent=self)
        for name, value in zip(names, values):
            new_env.define(name, value)
        return new_env
```

#### 全局环境

解释器启动时创建全局环境（`global_env`），注入所有内置函数和特殊形式。模块加载时创建以 `global_env` 为父环境的模块环境，确保模块可以访问内置函数但不会污染全局命名空间。

```python
def create_global_env() -> Environment:
    env = Environment()
    register_builtins(env)      # 算术、比较、列表等
    register_graphics(env)      # 图元函数
    register_transform(env)     # 矩阵函数
    register_sprites(env)       # 语义组件（角色/物品/UI/场景）
    register_style(env)         # 风格与调色板
    register_animation(env)     # 动画函数
    register_skeleton(env)      # 骨骼与IK
    return env
```

### 5.6 求值器 (Evaluator)

文件：`pml/evaluator.py`

求值器是解释器的核心，采用经典的 tree-walking 方式——递归遍历 AST 并在环境中求值。

#### 主循环

```python
def evaluate(expr: Expr, env: Environment) -> Any:
    # 1. 原子值直接返回
    if is_self_evaluating(expr):   # int, float, str, bool, None
        return expr
    
    # 2. 符号在环境中查找
    if isinstance(expr, Symbol):
        return env.lookup(expr.name)
    
    # 3. 列表 → 特殊形式或函数调用
    if isinstance(expr, list) and len(expr) > 0:
        head = expr[0]
        
        # 3a. 特殊形式
        if isinstance(head, Symbol) and head.name in SPECIAL_FORMS:
            return SPECIAL_FORMS[head.name](expr, env)
        
        # 3b. 宏调用（应在 Expander 阶段处理，此处作为后备）
        if isinstance(head, Symbol):
            val = try_lookup(head.name, env)
            if isinstance(val, Macro):
                expanded = val.expand(expr[1:])
                return evaluate(expanded, env)
        
        # 3c. 普通函数调用
        func = evaluate(head, env)
        args, kwargs = evaluate_arguments(expr[1:], env)
        return apply(func, args, kwargs)
    
    raise SyntaxError(f"Cannot evaluate: {expr}")
```

#### 特殊形式处理

每个特殊形式是一个 `(expr, env) → value` 的函数：

```python
SPECIAL_FORMS = {}

def eval_if(expr, env):
    # (if <cond> <then> <else>)
    condition = evaluate(expr[1], env)
    if is_truthy(condition):
        return evaluate(expr[2], env)
    else:
        return evaluate(expr[3], env) if len(expr) > 3 else None

SPECIAL_FORMS["if"] = eval_if

def eval_define(expr, env):
    if isinstance(expr[1], list):
        # 函数定义语法糖: (define (name params...) body)
        name = expr[1][0].name
        params = [p.name for p in expr[1][1:]]
        body = expr[2:]
        proc = Procedure(params, body, env)
    else:
        name = expr[1].name
        proc = evaluate(expr[2], env)
    env.define(name, proc)
    return None

SPECIAL_FORMS["define"] = eval_define

def eval_lambda(expr, env):
    params = [p.name for p in expr[1]]
    body = expr[2:]
    return Procedure(params, body, env)  # 捕获当前环境形成闭包

SPECIAL_FORMS["lambda"] = eval_lambda
```

#### 函数应用

```python
class Procedure:
    """用户定义的函数（闭包）"""
    def __init__(self, params: list[str], body: list[Expr], closure_env: Environment):
        self.params = params
        self.body = body
        self.closure_env = closure_env
    
    def call(self, args: list[Any], kwargs: dict = None) -> Any:
        # 创建新环境，以闭包环境为父
        call_env = self.closure_env.extend(self.params, args)
        # 顺序执行函数体，返回最后一个表达式的值
        result = None
        for expr in self.body:
            result = evaluate(expr, call_env)
        return result

class BuiltinProcedure:
    """内置函数"""
    def __init__(self, name: str, fn: Callable):
        self.name = name
        self.fn = fn
    
    def call(self, args: list[Any], kwargs: dict = None) -> Any:
        return self.fn(*args, **kwargs)
```

#### 关键字参数处理

PML 的关键字参数在求值器层面做特殊处理：

```python
def evaluate_arguments(exprs: list[Expr], env: Environment) -> tuple[list, dict]:
    """分离位置参数和关键字参数"""
    args = []
    kwargs = {}
    i = 0
    while i < len(exprs):
        if isinstance(exprs[i], Keyword):
            # 关键字参数：当前是 Keyword，下一个是值
            key = exprs[i].name
            value = evaluate(exprs[i + 1], env)
            kwargs[key] = value
            i += 2
        else:
            args.append(evaluate(exprs[i], env))
            i += 1
    return args, kwargs
```

### 5.7 宏展开器 (Expander)

文件：`pml/expander.py`

宏展开器在求值之前运行，遍历 AST 并展开所有宏调用。

```python
class Expander:
    def __init__(self, env: Environment, max_depth: int = 256):
        self.env = env
        self.max_depth = max_depth
    
    def expand(self, expr: Expr, depth: int = 0) -> Expr:
        if depth > self.max_depth:
            raise MacroExpansionDepthError(
                f"Macro expansion exceeded max depth of {self.max_depth}")
        
        if not isinstance(expr, list) or len(expr) == 0:
            return expr
        
        head = expr[0]
        if isinstance(head, Symbol):
            val = try_lookup(head.name, self.env)
            if isinstance(val, Macro):
                # 展开宏，然后递归展开结果中的宏
                expanded = val.expand(expr[1:])
                return self.expand(expanded, depth + 1)
        
        # 非宏调用：递归展开子表达式
        return [self.expand(sub, depth) for sub in expr]
```

### 5.8 模块加载器 (ModuleLoader)

文件：`pml/module_loader.py`

```python
class ModuleLoader:
    def __init__(self, global_env: Environment):
        self.global_env = global_env
        self.cache: dict[str, Module] = {}    # 路径 → 模块缓存
        self.loading: set[str] = set()         # 正在加载的模块（循环检测）
    
    def load(self, path: str, from_file: str) -> Module:
        # 解析路径
        resolved = self.resolve_path(path, from_file)
        
        # 缓存命中
        if resolved in self.cache:
            return self.cache[resolved]
        
        # 循环依赖检测
        if resolved in self.loading:
            raise CircularImportError(f"Circular import detected: {resolved}")
        self.loading.add(resolved)
        
        # 创建模块环境
        module_env = Environment(parent=self.global_env)
        
        # 解析并求值模块文件
        source = read_file(resolved)
        tokens = Lexer(source, resolved).tokenize()
        ast = Parser(tokens).parse()
        
        # 提取 provide 声明
        exports = self.extract_provides(ast)
        
        # 在模块环境中求值
        for expr in ast:
            evaluate(expr, module_env)
        
        module = Module(name=path_to_name(path), env=module_env, exports=exports)
        self.cache[resolved] = module
        self.loading.discard(resolved)
        return module
    
    def resolve_path(self, path: str, from_file: str) -> str:
        """按优先级解析模块路径"""
        # 1. 相对于当前文件
        candidate = os.path.join(os.path.dirname(from_file), path)
        if os.path.exists(candidate):
            return os.path.abspath(candidate)
        # 2. PML_PATH 环境变量
        # 3. 标准库目录
        # ...
```

### 5.9 渲染后端

文件：`pml/backend/`

渲染后端采用抽象基类模式，允许插拔不同的图形库。

```python
from abc import ABC, abstractmethod

class RenderBackend(ABC):
    @abstractmethod
    def create_surface(self, width: int, height: int, bg_color: str) -> Any:
        """创建绘图表面"""
    
    @abstractmethod
    def draw(self, surface: Any, obj: GraphicObject) -> None:
        """绘制单个图形对象（递归处理 group）"""
    
    @abstractmethod
    def save_image(self, surface: Any, path: str, format: str) -> None:
        """保存为静态图像"""
    
    @abstractmethod
    def save_animation(self, frames: list[Any], path: str, 
                       format: str, fps: int) -> None:
        """保存为动画（GIF/MP4）"""
```

初始实现优先支持 Pillow 后端（跨平台、易安装），后续添加 Cairo 后端（更高质量的矢量渲染）。

#### 渲染流水线

```
GraphicObject 树
    │
    ▼
┌────────────────┐
│  场景扁平化     │  递归展开 group，应用变换矩阵
└────┬───────────┘
     │ list[DrawableItem]
     ▼
┌────────────────┐
│  排序（画家算法） │  按 z-order 排序
└────┬───────────┘
     │
     ▼
┌────────────────┐
│  后端绘制       │  调用 Cairo/Pillow API 逐个绘制
└────┬───────────┘
     │
     ▼
  输出文件
```


---

## 第六部分：错误处理

### 6.1 异常类型体系

```
PMLError (基类)
├── SyntaxError            词法/语法错误（括号不匹配、非法原子）
├── ImportError            模块加载错误
│   ├── FileNotFoundError  文件不存在
│   └── CircularImportError 循环依赖
├── TypeError              参数类型/数量错误
├── UnboundVariableError   未绑定变量
├── ResourceError          外部资源错误
│   └── ResourceNotFoundError  图片/字体文件缺失
├── IKNoSolutionError      IK 求解无法收敛
├── MacroExpansionDepthError  宏展开深度超限
├── AccessError            访问未导出的模块符号
└── AssertionError         断言失败
```

### 6.2 错误报告格式

所有错误输出遵循统一的结构化格式，便于 LLM 解析和人类阅读：

```
Error: <ErrorType> at <file>:<line>:<column>
Message: <人类可读的错误描述>
Context:
  <line-2> | ...
  <line-1> | ...
> <line>   | (circle 0 0 "abc")     ← 此处出错
  <line+1> | ...

Hint: <修复建议（可选）>
```

示例：

```
Error: TypeError at main.pml:5:3
Message: circle expected number for parameter 'r', got string "abc"
Context:
  3 | (canvas 800 600)
  4 | (define r "abc")
> 5 | (circle 100 100 r :fill "red")
       ^^^^^^^^^^^^^^^^^^^
Hint: Ensure 'r' is a number. Did you mean (string->number r)?
```

### 6.3 错误模式

**严格模式**（默认）：任何错误立即中止执行，抛出异常并附带完整上下文。

**宽容模式**：通过 `(set! *safe-mode* #t)` 开启。可恢复错误（如缺失图片）使用占位符替代并打印警告，不可恢复错误（如语法错误、类型错误）仍会中止执行。

```pml
(set! *safe-mode* #t)
(image "missing.png" 0 0)  ; 显示棋盘格占位符，打印警告，继续执行
```


---

## 第七部分：开发指南

### 7.1 开发环境

```bash
# 克隆仓库
git clone https://github.com/<org>/pml.git
cd pml

# 创建虚拟环境
python -m venv .venv
source .venv/bin/activate  # macOS/Linux
.venv\Scripts\activate     # Windows

# 安装开发依赖
pip install -e ".[dev]"
```

开发依赖：

| 包 | 用途 |
|---|------|
| `pytest` | 测试框架 |
| `pytest-cov` | 覆盖率报告 |
| `ruff` | Linting + 格式化 |
| `mypy` | 静态类型检查 |
| `pre-commit` | Git 提交钩子 |

### 7.2 编码约定

**格式**：遵循 PEP 8，由 `ruff` 自动执行。行宽限制 100 字符。

**类型标注**：所有公开函数必须有完整的类型标注。内部辅助函数鼓励标注但不强制。

**命名**：

| 范围 | 约定 | 示例 |
|------|------|------|
| 类名 | PascalCase | `GraphicObject`, `AffineTransform` |
| 函数/变量 | snake_case | `evaluate`, `global_env` |
| 常量 | UPPER_SNAKE_CASE | `MAX_MACRO_DEPTH`, `DEFAULT_FPS` |
| PML 内置函数 | 保持 PML 名称 | `stroke_width`（Python 侧对应 `stroke-width`） |
| 私有成员 | 单前缀下划线 | `_resolve_path`, `_cache` |

**Docstring**：所有公开模块、类、函数使用 Google 风格 docstring：

```python
def evaluate(expr: Expr, env: Environment) -> Any:
    """Evaluate a PML expression in the given environment.
    
    Args:
        expr: The AST node to evaluate.
        env: The environment for variable lookup.
    
    Returns:
        The evaluated result.
    
    Raises:
        UnboundVariableError: If a symbol is not found in any scope.
        TypeError: If a function receives incompatible argument types.
    """
```

### 7.3 测试策略

#### 单元测试

每个模块对应一个测试文件，测试粒度到函数级别：

```python
# tests/test_evaluator.py
import pytest
from pml.evaluator import evaluate
from pml.environment import Environment, create_global_env

class TestArithmetic:
    def setup_method(self):
        self.env = create_global_env()
    
    def test_addition(self):
        assert evaluate([Symbol("+"), 1, 2], self.env) == 3
    
    def test_nested_arithmetic(self):
        # (* (+ 1 2) (- 10 3)) = 3 * 7 = 21
        expr = [Symbol("*"),
                [Symbol("+"), 1, 2],
                [Symbol("-"), 10, 3]]
        assert evaluate(expr, self.env) == 21

class TestDefine:
    def test_define_variable(self):
        env = create_global_env()
        evaluate([Symbol("define"), Symbol("x"), 42], env)
        assert env.lookup("x") == 42
```

#### 集成测试

使用 `.pml` 文件作为测试 fixture，验证端到端行为：

```python
# tests/test_integration.py
class TestExamples:
    def test_hello_circle(self):
        """tests/fixtures/hello_circle.pml 应生成有效的 PNG 文件"""
        output = run_pml("tests/fixtures/hello_circle.pml")
        assert output.exists()
        assert is_valid_png(output)
```

#### 运行测试

```bash
# 全量测试
pytest

# 带覆盖率
pytest --cov=pml --cov-report=html

# 单文件
pytest tests/test_evaluator.py -v

# 单测试
pytest tests/test_evaluator.py::TestArithmetic::test_addition -v
```

### 7.4 Git 约定

**分支策略**：基于 trunk 的开发模式，短生命周期的 feature 分支。

**提交消息**：遵循 Conventional Commits 格式：

```
<type>(<scope>): <description>

[optional body]
```

`type` 取值：`feat`、`fix`、`refactor`、`test`、`docs`、`chore`、`perf`。

`scope` 对应模块名：`lexer`、`parser`、`eval`、`graphics`、`anim`、`ik`、`module`、`macro`。

示例：

```
feat(eval): implement let and let* special forms

Add support for local variable bindings with let (parallel) and
let* (sequential). Both create a child environment extending the
current scope.
```

### 7.5 版本规范

遵循 Semantic Versioning（语义化版本）：

- `0.x.y`：初始开发阶段，API 可能频繁变动。
- `1.0.0`：语言规范稳定，核心功能完备。
- 后续版本按兼容性规则递增。


---

## 第八部分：实现路线图

### Phase 0：基础设施（1-2 周）

- 项目骨架搭建（目录结构、pyproject.toml、CI）
- Lexer：完整的词法分析，覆盖所有 Token 类型
- Parser：S-表达式解析，括号匹配校验

**交付物**：可以解析任意 `.pml` 文件为 AST。

### Phase 1：核心求值（2-3 周）

- Environment：作用域链、define / set! / lookup
- Evaluator：自求值、符号查找、函数调用
- 特殊形式：`if`、`define`、`lambda`、`begin`、`set!`、`let`、`let*`
- 内置函数：算术、比较、逻辑、列表操作、字符串操作、类型判断、IO

**交付物**：可以执行纯计算类 PML 程序（不涉及图形）。

### Phase 2：图形图元 + Sprite 输出（3-4 周）

- GraphicObject 数据模型
- 图元内置函数：`circle`、`rect`、`ellipse`、`line`、`polygon`、`path`、`text`、`image`
- `group` 组合
- `canvas` / `sprite-canvas` 画布设置（含透明背景、锚点）
- 仿射变换：`translate`、`rotate`、`scale`、`shear`、`compose`
- 渲染后端：Pillow 实现（PNG with alpha / JPG 输出）
- Sprite 输出：画框裁切、锚点元数据、`render` 支持 `:sheet` / `:frame-size` / `:anchor`
- `render-set` 多分辨率输出

**交付物**：可以编写并渲染带透明通道和锚点的静态 sprite。

### Phase 3：风格系统 + 基础语义组件（3-4 周）

- StyleDescriptor 数据结构与 `define-style` / `use-style`
- 预定义风格模板：`cel`、`pixel`、`flat`
- Palette 调色板系统：`define-palette` / `palette-ref`
- 风格传播机制（父级 → 子组件）
- 组件注册器与参数验证器（`registry.py` + `validator.py`）
- 首批语义组件：`body`、`head`、`anime-eyes`、`mouth`、`hair`
- `character` 角色组装器（基础版）

**交付物**：可以用 `(character :hair "spiky" :eyes "shoujo" ...)` 生成一个完整角色 sprite。

### Phase 4：扩展语义组件（3-4 周）

- `outfit` 服装组件
- 物品组件：`weapon`、`potion`、`chest`、`generic-item`
- UI 组件：`button`、`panel`、`health-bar`、`icon`
- 场景组件：`tile`、`decoration`、`background`
- `character` 组装器完善（`pose`、`direction`、`expression` 支持）
- 更多风格模板：`soft`、`chibi`、`outline`
- 标准库 `.pml` 文档：各组件的 API 参考

**交付物**：完整的语义组件库，覆盖角色 / 物品 / UI / 场景四大领域。

### Phase 5：模块与宏（1-2 周）

- ModuleLoader：路径解析、缓存、循环检测
- `provide` / `import` 特殊形式
- 标准库：`math.pml`、`color.pml`
- 宏系统：`defmacro`、quasiquote 展开、`gensym`

**交付物**：模块化开发能力，用户可自定义组件。

### Phase 6：LLM 接口（1-2 周）

- `PMLRuntime` API 类：`execute`、`render_sprite`、`validate`
- 组件发现 API：`list_components`、`preview_params`
- 结构化错误反馈（JSON 格式，含修复建议）
- HTTP 服务封装（可选，方便远程调用）

**交付物**：外部 LLM 智能体可通过 API 生成 sprite。

### Phase 7：动画系统（2-3 周）

- 动画引擎：时间轴、帧循环
- `animate` 属性动画、缓动函数
- 颜色插值
- `parallel` / `sequence` 组合动画
- 播放控制：`play`、`stop`、`pause`、`seek`
- `render` 支持 GIF / MP4 / 帧序列输出
- `every-frame` 钩子

**交付物**：可以创建和导出动画 sprite sheet 和 GIF。

### Phase 8：骨骼与 IK（2-3 周）

- `defskeleton` 骨骼模板定义
- `instantiate-skeleton` 实例化
- FABRIK 求解器
- CCD 求解器
- `bind-skin` 皮肤绑定
- `joint-position` 查询
- 骨骼动画与 sprite sheet 输出的集成

**交付物**：骨骼动画角色，可输出 sprite sheet。

### Phase 9：打磨与扩展（持续）

- REPL 交互环境
- Cairo 后端（高质量矢量渲染）
- SVG 输出后端
- 错误信息优化（上下文代码、修复建议）
- 性能优化（大型场景的渲染）
- 组件库扩充（更多发型、服装、武器变体）
- 社区贡献的组件包生态
- 文档完善与示例库扩充
- 并发支持（可选）


---

## 附录 A：完整语法速查表

### 特殊形式

```
(quote <expr>)                   → 字面量
(if <cond> <then> <else>)        → 条件
(cond (<test> <expr>) ... )      → 多分支条件
(define <name> <expr>)           → 定义
(define (<name> <params>) <body>) → 函数定义
(lambda (<params>) <body>)       → 匿名函数
(let ((<n> <e>) ...) <body>)     → 局部绑定
(let* ((<n> <e>) ...) <body>)    → 顺序局部绑定
(begin <expr> ...)               → 顺序执行
(set! <name> <expr>)             → 修改变量
(do ((<v> <init> <step>) ...)    → 迭代
    (<test> <result>) <body>)
(load <filename>)                → 加载文件
(provide <symbol> ...)           → 模块导出
(import <file> [as <prefix>])    → 模块导入
(defmacro <name> (<p>) <body>)   → 宏定义
(defskeleton <name> ...)         → 骨骼定义
```

### 内置函数分类速查

```
; 算术
+  -  *  /  %  abs  min  max  floor  ceil  round  sqrt  pow
sin  cos  tan  atan2

; 比较
=  <  >  <=  >=  eq?  equal?

; 逻辑
and  or  not

; 列表
cons  car  cdr  list  length  append  reverse
map  filter  reduce  nth  range  null?  list?

; 字符串
string-append  string-length  substring  string-ref
number->string  string->number  format

; 类型判断
number?  string?  symbol?  boolean?  procedure?
keyword?  graphic-object?  matrix?

; IO
print  println  error  assert  typeof

; 图元
circle  rect  ellipse  line  polygon  path  text  image  group

; 画布与渲染
canvas  render

; 变换
translate  rotate  scale  shear  compose
transform  identity-matrix  matrix-inverse  matrix-apply  matrix?

; 动画
animate  parallel  sequence  play  stop  pause  seek
on-finished  animation-state  every-frame

; 骨骼与IK
defskeleton  instantiate-skeleton  ik-solve
bind-skin  joint-position

; 宏辅助
gensym

; 风格与调色板
define-style  use-style  define-palette  palette-ref

; 语义组件 — 角色
character  body  head  anime-eyes  mouth  hair  outfit

; 语义组件 — 物品
weapon  potion  chest  generic-item

; 语义组件 — UI
button  panel  health-bar  icon

; 语义组件 — 场景
tile  decoration  background

; Sprite 输出
sprite-canvas  render-set
```

### 关键字参数速查

```
; 图元通用
:fill  :stroke  :stroke-width  :transform

; text 专用
:font-family  :font-size

; canvas
:bg

; render
:format  :fps  :duration

; animate
:ease  :repeat  :delay

; ik-solve
:method  :iterations  :tolerance

; instantiate-skeleton
:name

; joint (在 defskeleton 内)
:pos  :length  :angle  :min  :max

; sprite-canvas
:anchor  :padding

; render (sprite 扩展)
:sheet  :frame-size  :spacing

; render-set
:content  :scales  :base-size

; character 组装器
:body  :head  :hair  :outfit  :accessory  :weapon
:pose  :direction  :expression  :style  :palette  :scale

; 风格
:outline-width  :outline-color  :outline-style  :shading
:shadow  :highlight  :pixel-size  :anti-alias  :corner-radius
```

## 附录 B：PML 示例代码

### B.1 基础图形

```pml
; 简单的彩色圆形
(canvas 400 400 :bg "#f5f5f5")
(circle 200 200 80 :fill "#3498db" :stroke "#2c3e50" :stroke-width 3)
(render "blue_circle.png")
```

### B.2 函数与组合

```pml
; 绘制一组渐变大小的同心圆
(canvas 400 400)

(define (concentric-circles cx cy n max-r)
  (if (= n 0)
      (group)  ; 空组
      (let ((r (* max-r (/ n 10.0))))
        (group
          (circle cx cy r 
                  :fill (format "rgba(52, 152, 219, ~a)" (/ n 10.0)))
          (concentric-circles cx cy (- n 1) max-r)))))

(concentric-circles 200 200 10 150)
(render "concentric.png")
```

### B.3 动画

```pml
; 弹跳小球动画
(canvas 600 400 :bg "#2c3e50")

(define ball (circle 50 350 20 :fill "#e74c3c"))
(define anim
  (sequence
    (animate ball 'y 350 50 0.5 :ease 'quad-out)
    (animate ball 'y 50 350 0.5 :ease 'bounce)))

(play (sequence (parallel anim) 
                (animate ball 'x 50 550 2.0 :ease 'linear)))
(render "bounce.gif" :format 'gif :fps 30)
```

### B.4 骨骼 IK

```pml
; 可交互的手臂 IK 演示
(canvas 600 400 :bg "#ecf0f1")

(defskeleton arm (x y)
  (joint shoulder :pos (0 0)  :length 60 :angle 0 :min -1.5 :max 1.5)
  (joint elbow    :pos (60 0) :length 50 :angle 0 :min 0    :max 2.5)
  (joint hand     :pos (50 0) :length 0))

(define my-arm (instantiate-skeleton arm (200 200) :name "demo"))

; 每帧求解 IK，使手跟随目标点
(define target-x 350)
(define target-y 150)

(every-frame (lambda ()
  (ik-solve my-arm 'hand target-x target-y :method 'fabrik)))

; 绘制骨骼
(define (draw-skeleton skel)
  (let ((s (joint-position skel 'shoulder))
        (e (joint-position skel 'elbow))
        (h (joint-position skel 'hand)))
    (group
      (line (car s) (cadr s) (car e) (cadr e) :stroke "#2c3e50" :stroke-width 4)
      (line (car e) (cadr e) (car h) (cadr h) :stroke "#2c3e50" :stroke-width 4)
      (circle (car s) (cadr s) 6 :fill "#e74c3c")
      (circle (car e) (cadr e) 5 :fill "#3498db")
      (circle (car h) (cadr h) 4 :fill "#2ecc71"))))

(draw-skeleton my-arm)
(render "ik_demo.gif" :format 'gif :duration 2.0 :fps 30)
```

### B.5 角色 Sprite 生成

```pml
; 使用语义组件库生成一个动漫角色 sprite
(import "sprites/styles/palettes.pml" as palettes)

; 定义调色板
(define-palette "shadow-mage"
  :primary   "#1a1a2e"
  :secondary "#16213e"
  :accent    "#e94560"
  :skin      "#f5deb3"
  :hair      "#0f3460"
  :outline   "#0a0a1a")

; 生成角色
(sprite-canvas 128 128 :anchor 'center-bottom)

(define hero
  (character
    :body (body :height 140 :build "slim" :proportions 'anime)
    :head (head :shape "oval" :skin (palette-ref "skin"))
    :eyes (anime-eyes :style "shoujo" :color (palette-ref "accent")
                      :highlight #t :lashes "long")
    :mouth (mouth :style "neutral")
    :hair (hair :style "long" :color (palette-ref "hair")
                :bangs "side" :accessory "ribbon")
    :outfit (outfit :top "robe" :bottom "long-skirt" :shoes "boots"
                    :color-top (palette-ref "primary")
                    :color-bottom (palette-ref "secondary")
                    :detail "rune")
    :weapon (weapon :type "staff" :size "large"
                    :material "crystal" :element "dark")
    :pose 'standing
    :direction 'front
    :expression 'confident
    :style 'cel
    :palette "shadow-mage"))

hero
(render "shadow_mage.png" :anchor 'center-bottom)
(render-set "shadow_mage" :content hero
            :scales '(1 2 4) :base-size (128 128))
```

### B.6 Sprite Sheet 输出

```pml
; 生成角色行走动画的 sprite sheet
(sprite-canvas 64 64 :anchor 'center-bottom)

(define walk-frames
  (map (lambda (i)
         (character
           :pose 'walking
           :direction 'right
           :style 'pixel
           :scale 0.5
           :hair (hair :style "short" :color "#8B4513")
           :outfit (outfit :top "t-shirt" :bottom "pants"
                           :color-top "#3498db" :color-bottom "#2c3e50")))
       (range 0 8)))

(render "hero_walk.png"
  :format 'png
  :sheet (4 2)
  :frame-size (64 64)
  :frames walk-frames)
; 输出 hero_walk.png + hero_walk.meta.json
```

### B.7 UI 控件集

```pml
; 生成一套游戏 UI 控件 sprite
(sprite-canvas 256 512 :bg "transparent")

(define ui-kit
  (group
    ; 按钮
    (button :label "Start" :width 120 :height 40 :style 'rounded
            :color "#27ae60" :text-color "white"
            :transform (translate 10 10))
    (button :label "Settings" :width 120 :height 40 :style 'rounded
            :state 'hover :color "#2980b9" :text-color "white"
            :transform (translate 10 60))
    
    ; 血条和蓝条
    (health-bar :value 0.75 :width 150 :height 16 :style 'gradient
                :color "#e74c3c" :transform (translate 10 120))
    (health-bar :value 0.45 :width 150 :height 16 :style 'gradient
                :color "#3498db" :transform (translate 10 145))
    
    ; 图标
    (icon :type 'heart :size 24 :color "#e74c3c" :style 'detailed
          :transform (translate 10 180))
    (icon :type 'star :size 24 :color "#f1c40f" :style 'detailed
          :transform (translate 40 180))
    (icon :type 'coin :size 24 :color "#f39c12" :style 'detailed
          :transform (translate 70 180))
    
    ; 面板
    (panel :width 200 :height 120 :style 'ornate
           :title "Inventory" :color "#2c3e50"
           :transform (translate 10 220))))

ui-kit
(render "ui_kit.png")
```
