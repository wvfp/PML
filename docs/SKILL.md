---
name: pml
description: |
  PML (Picture Markup Language) — C++23 Lisp-style DSL for code-to-image generation.
  This skill teaches how to organize PML projects, the correct order to call builtins,
  and provides complete project examples and a full builtin reference.
  Triggers:
  - "write pml code", "generate pml", "use pml"
  - Any request involving PML language syntax, APIs, or rendering pipeline
  - Creating images, animations, tilemaps, shaders, game assets via PML
---

# PML — 图片标记语言 实战指南

PML 是基于 S-表达式的 DSL，用代码生成 2D/3D 图像、动画和游戏素材。
C++23 实现，Skia GPU 后端渲染，支持 PNG/GIF 输出。

---

## 快速识别

| 项目 | 说明 |
|------|------|
| 后缀 | `.pml` |
| 语法 | `(function arg1 arg2 :kwarg val)` |
| 编码 | UTF-8（CJK 文字受 Skia font fallback 限制） |
| 渲染 | Skia GPU / GIF CPU |
| 构建 | `cmake --preset debug && cmake --build --preset debug` |
| 运行 | `pml file.pml` / `pml`(REPL) / `pml file.pml --watch` |

---

## 一、PML 项目组织模式

通过学习 examples/09-projects/ 中的真实项目，PML 项目有以下三种组织模式：

### 模式 1：单文件项目（简单场景）

适合 100 行以内、一个渲染主题的小作品。

```
project/
├── main.pml                   # 全部代码在一个文件里
└── output.png
```

**典型应用**：程序化风景、简单角色展示、Logo 渲染

**参考项目**：`examples/09-projects/04_generative_landscape/`

### 模式 2：主文件 + 模块（中等复杂）

将绘制函数和数据拆分到模块中，通过 `import`/`provide` 组织。

```
project/
├── main.pml                   # 入口：canvas→import→add→render
├── tiles.pml                  # 提供瓦片/物体绘制函数
├── map_data.pml               # 地图数据单独管理
├── decorations.pml            # 装饰元素绘制函数
├── isometric.pml              # 坐标转换算法
└── output.png
```

**典型应用**：瓦片地图、Logo 组合、角色动画

**模块接口约定**：
- `provide` 放在文件顶部，一目了然
- 模块函数全部通过 `prefix/sym` 方式引用，避免命名冲突
- 数据文件（如 map_data.pml）只 `provide` 常量，不包含逻辑

**参考项目**：
- `examples/09-projects/01_parallax_platformer/` — 主文件 + tiles 模块
- `examples/09-projects/05_doubao_logo/` — 5 个模块分别管理不同组件
- `examples/09-projects/07_isometric_tilemap/` — 4 个模块（logic + data + decor + main）

### 模式 3：Shader 增强项目（高级）

在模式 2 基础上增加 SkSL 着色器模块，实现程序化纹理。

```
project/
├── main.pml                   # 入口
├── shaders.pml                # 提供多个 SkSL shader handle
├── isometric.pml              # 坐标/渲染算法
├── decorations.pml            # 装饰物绘制
└── output.png
```

**典型应用**：动漫风格等距场景、程序化地形、噪声纹理

**参考项目**：`examples/09-projects/11_anime_wilderness/` — shaders.pml 定义 4 个噪声着色器、isometric.pml 渲染带 shader 的 3D 瓦片

---

## 二、关键词调用顺序（Pipeline）

PML 脚本从上到下顺序执行。不同项目类型有不同的内置函数调用顺序：

### 2.1 静态图像项目

```
第 1 步：set-backend!     →  选择渲染后端（"skia"）
第 2 步：canvas           →  创建画布（尺寸、背景色）
第 3 步：import           →  加载模块（如需）
第 4 步：define           →  定义数据、常量、配置参数
第 5 步：define           →  定义绘制函数（递归或 map/filter/reduce）
第 6 步：define           →  创建图形对象（circle, rect, polygon...）
第 7 步：add              →  添加图形到画布
第 8 步：render           →  渲染输出文件
```

**关键规则**：
- `set-backend!` 必须在 `canvas` 之前
- `canvas` 必须在 `add` 和 `render` 之前
- `import` 可在 `canvas` 之前或之后（模块只提供函数定义）
- `add` 的顺序决定了图层前后关系（后添加的在上层）

### 2.2 动画项目

```
第 1 步：set-backend!
第 2 步：canvas
第 3 步：define 图形对象 + add 到画布
第 4 步：animate         →  创建补间动画（指定目标属性）
第 5 步：parallel/sequence →  组合多个动画
第 6 步：play            →  开始播放
第 7 步：render :fps N   →  以 GIF 格式输出
```

### 2.3 Composition（图层合成）项目

```
第 1 步：set-backend!
第 2 步：make-composition    →  创建合成场景
第 3 步：make-layer          →  为每个元素创建图层
第 4 步：composition-add     →  将所有图层添加到合成
第 5 步：composition-render  →  渲染合成场景
```

### 2.4 Tilemap 项目（内置 tileset 系统）

```
第 1 步：set-backend!
第 2 步：define-tileset       →  定义瓦片集（每个瓦片 ID + 图形）
第 3 步：make-tilemap         →  创建瓦片地图（指定行列数、投影类型、层数）
第 4 步：tilemap-set!         →  填充地图数据（layer, col, row, tile-id）
第 5 步：render-tilemap       →  渲染地图
```

### 2.5 等距瓦片项目（自定义几何体系）

等距瓦片通常不通过内置 tilemap 系统，而是用自定义几何函数：

```
第 1 步：set-backend! / canvas
第 2 步：define 配置参数       →  tile-w, tile-h, tile-depth, origin-x, origin-y
第 3 步：import 等距模块       →  iso-to-screen, draw-diamond-tile 等
第 4 步：define 地图数据       →  二维列表网格
第 5 步：递归渲染地面瓦片      →  pass 1：从远到近绘制 3D 瓦片
第 6 步：递归渲染装饰物        →  pass 2：在瓦片之上绘制树木/石头/花朵
第 7 步：render
```

### 2.6 Shader 增强项目

```
第 1 步：set-backend! / canvas
第 2 步：define 着色器源码     →  SkSL 字符串
第 3 步：shader               →  编译为 shader handle
第 4 步：define 图形对象
第 5 步：apply-shader!        →  对着色器作用域内的图形应用 shader
第 6 步：add 到画布
第 7 步：render
```

---

## 三、完成项目实例剖析

### 实例 1：视差卷轴平台场景（模块化模式）

**文件 1：main.pml** — 入口
```scheme
(set-backend! "skia")
(canvas 640 360 :bg "#87CEEB")
(import "tiles.pml" as t)

; 场景元素直接 add
(add (circle 540 80 45 :fill "#FFD700" :stroke "#FFA500" :stroke-width 3))
(add (polygon (list '(0 360) '(160 240) '(320 300) '(480 220) '(640 280) '(640 360))
              :fill "#5D8AA8"))

; 使用模块函数
(define (ground-row y count size start-x)
  (if (> count 0)
      (begin
        (add (t/draw-ground-tile start-x y size))
        (ground-row y (- count 1) size (+ start-x size)))))
(ground-row 320 11 64 -32)

(add (t/draw-tree 110 320 1.3))
(add (t/draw-tree 520 320 1.0))

; 精灵组件 + 文字
(define hero (character :expression 'happy :style 'cel :scale 0.9))
(add (translate-object hero 300 300))
(add (text 320 30 "Parallax Platformer" :fill "#FFFFFF" :font-size 24 :align 'center))

(render "01_parallax_platformer.png")
```

**文件 2：tiles.pml** — 模块
```scheme
(provide draw-ground-tile draw-tree)

(define (draw-ground-tile x y size)
  (group (rect x y size size :fill "#8B4513" :stroke "#5D4037" :stroke-width 2)
         (rect (+ x 4) (+ y 4) (- size 8) 6 :fill "#A0522D")
         (rect (+ x 4) (+ y 16) (- size 8) 4 :fill "#A0522D")))

(define (draw-tree x y scale)
  (define trunk-w (* 8 scale))
  (define trunk-h (* 24 scale))
  (define canopy-r (* 16 scale))
  (group (rect (- x (/ trunk-w 2)) (- y trunk-h) trunk-w trunk-h :fill "#8B4513")
         (circle x (- y trunk-h (/ canopy-r 2)) canopy-r :fill "#228B22")
         (circle (- x (* 10 scale)) (- y trunk-h (* scale 6)) (* 12 scale) :fill "#32CD32")
         (circle (+ x (* 10 scale)) (- y trunk-h (* scale 6)) (* 12 scale) :fill "#32CD32")))
```

### 实例 2：卡牌游戏 UI（Composition 模式）

```scheme
(set-backend! "skia")

(define comp (make-composition "card-game-ui" 640 400 :bg "#1A1A2E"))

; 背景层
(define bg-panel (make-layer "bg" (rect 0 0 640 400 :fill "#23243A" :rx 0) :offset '(0 0)))

; 标题层
(define title (make-layer "title"
                           (text 320 50 "Card Battle" :fill "#FFD700" :font-size 32 :align 'center)
                           :offset '(0 0)))

; 血条层：背景 + 填充 + 图标
(define hp-bg (make-layer "hp-bg" (rect 60 90 240 24 :fill "#3A3B52" :rx 12) :offset '(0 0)))
(define hp-bar (make-layer "hp-bar" (rect 60 90 180 24 :fill "#E74C3C" :rx 12) :offset '(0 0)))
(define hp-icon (make-layer "hp-icon" (icon :type 'heart :size 28 :color "#E74C3C") :offset '(34 88)))

; 卡牌层 — 带投影滤镜
(define card-1 (make-layer "card-1"
                            (group (rect 0 0 120 160 :fill "#34495E" :stroke "#FFFFFF"
                                         :stroke-width 2 :rx 8)
                                   (circle 60 60 35 :fill "#E74C3C")
                                   (text 60 130 "Fire" :fill "#FFFFFF" :font-size 16 :align 'center))
                            :offset '(80 150)
                            :filter (drop-shadow :dx 4 :dy 6 :blur 8 :color "#000000" :opacity 0.5)))

; 按钮层
(define btn (make-layer "btn"
                         (button :label "END TURN" :width 140 :height 42
                                 :style 'rounded :color "#27AE60" :state 'normal)
                         :offset '(250 340)
                         :filter (drop-shadow :dx 2 :dy 4 :blur 6 :color "#000000" :opacity 0.4)))

; 组装合成 → 渲染
(set! comp (composition-add comp bg-panel title hp-bg hp-bar hp-icon card-1 card-2 card-3 btn))
(composition-render comp "03_card_game_ui.png")
```

**关键点**：
- `make-composition` + `make-layer` 替代 `canvas` + `add`
- 每个 layer 有独立的 `:offset`、`:opacity`、`:filter`
- `composition-add` 一次性添加多个 layer，`composition-render` 输出

### 实例 3：行走循环动画（动画模式）

```scheme
(set-backend! "skia")
(canvas 360 260 :bg "#87CEEB")

(import "body.pml" as b)

(add (rect 0 220 360 40 :fill "#8B4513" :stroke "#5D4037" :stroke-width 2))

(define walker-1 (b/make-walker 30 170 0.85 "warm-skin"))
(define walker-2 (b/make-walker 30 205 0.65 "dark-hero"))
(add walker-1)
(add walker-2)

; 创建动画：水平移动 + 垂直颠簸
(define w1-move (animate walker-1 'x 30 310 2.0 :ease 'linear))
(define w1-bob  (animate walker-1 'y 170 160 0.5 :ease 'ease-in-out :repeat 4))

(define w2-move (animate walker-2 'x 30 310 2.5 :ease 'linear))
(define w2-bob  (animate walker-2 'y 205 198 0.625 :ease 'ease-in-out :repeat 4))

; 组合动画
(parallel w1-move w1-bob w2-move w2-bob)
(play)

(render "02_walk_cycle_character.gif" :fps 12)
```

**动画关键语法**：
- `(animate graphic-obj 'property from to duration :ease EASE-TYPE)`
- `(parallel ...)` 同时播放 → `(sequence ...)` 顺序播放
- `(play)` 开始整个时间线
- GIF 输出用 `render :fps N`

### 实例 4：动漫风格等距场景（Shader + 模块模式）

四个文件协作：`main.pml` + `shaders.pml` + `isometric.pml` + `decorations.pml`

**主文件 main.pml** 的关键结构：
```scheme
; 步骤 1：配置
(set-backend! "skia")
(canvas 1600 1200 :bg "#141E28")
(import "shaders.pml" as sh)
(import "isometric.pml" as iso)
(import "decorations.pml" as deco)

; 步骤 2：定义参数
(define tile-w 80) (define tile-h 40) (define tile-depth 20)
(define origin-x 800) (define origin-y 280)

; 步骤 3：定义地图数据（12×12 网格，0=草地 1=泥土 2=石头 3=沙地）
(define map-grid (list ...))

; 步骤 4：Pass 1 — 渲染地面（使用 shader 做赛璐璐纹理）
(iso/render-tilemap map-grid tile-w tile-h tile-depth
                    origin-x origin-y outline outline-w
                    sh/grass-shader sh/dirt-shader
                    sh/stone-shader sh/sand-shader)

; 步骤 5：Pass 2 — 渲染装饰物（树木、灌木、花朵、岩石）
(iso/render-tilemap-decorations map-grid decorations tile-w tile-h
                                origin-x origin-y outline outline-w
                                deco/draw-decoration)

; 步骤 6：输出
(render "anime_wilderness.png")
```

**shaders.pml** — SkSL 着色器模块：
```scheme
(provide grass-shader dirt-shader stone-shader sand-shader)

; 共享噪声函数
(define cel-noise
  (string-append
    "float h = fract(sin(dot(floor(p * 0.6), float2(127.1, 311.7)) + 0.001) * 43758.5453);\n"
    "float h2 = ...\n"
    "float band = floor(n * 3.0 + 0.1) / 2.0;\n"))

; 草地着色器 — 3 色阶赛璐璐
(define grass-shader
  (shader
    (string-append
      "half4 main(float2 p) {\n"
      cel-noise
      "  half3 c = band < 0.3 ? half3(0.28, 0.50, 0.22)\n"
      "        : band < 0.7 ? half3(0.38, 0.60, 0.28)\n"
      "        : half3(0.50, 0.69, 0.38);\n"
      "  return half4(c, 1.0);\n"
      "}\n")))
; ...dirt-shader, stone-shader, sand-shader 同理
```

**isometric.pml** — 等距坐标 + 3D 瓦片渲染：
```scheme
(provide iso-to-screen draw-diamond-tile draw-tile-with-shader render-tilemap ...)

; 等距坐标 → 屏幕坐标
(define (iso-to-screen tx ty tile_w tile_h origin_x origin_y)
  (list
    (+ origin_x (* (/ tile_w 2) (- tx ty)))
    (+ origin_y (* (/ tile_h 2) (+ tx ty)))))

; 3D 瓦片：左侧面 + 右侧面 + 顶面（带 shader）
(define (draw-tile-with-shader cx cy w h depth shader-handle
                                left-fill right-fill stroke sw)
  (group
    (draw-tile-left cx cy w h depth left-fill stroke sw)
    (draw-tile-right cx cy w h depth right-fill stroke sw)
    (apply-shader! (draw-diamond-tile cx cy w h "#FFFFFF" stroke sw)
                   shader-handle)))
```

---

## 四、递归数据结构模式

PML 没有 while/for 循环，使用递归处理数据。以下是三种常见递归模式：

### 4.1 递归生成列表
```scheme
(define (range-list start end)
  (if (>= start end)
      '()
      (cons start (range-list (+ start 1) end))))
; (range-list 0 5) → (0 1 2 3 4)
```

### 4.2 递归处理列表（car/cdr 模式）
```scheme
(define (render-tilemap-iter map-grid tile_w tile_h depth ox oy stroke sw row col)
  (if (< row (length map-grid))
      (if (< col (length (car map-grid)))
          (begin
            (add (draw-tile (list-ref (list-ref map-grid row) col)
                            (iso-to-screen col row tile_w tile_h ox oy)
                            tile_w tile_h))
            (render-tilemap-iter map-grid tile_w tile_h depth ox oy stroke sw row (+ col 1)))
          (render-tilemap-iter map-grid tile_w tile_h depth ox oy stroke sw (+ row 1) 0))))
```

### 4.3 使用高阶函数替代递归
```scheme
; map：批量创建
(define circles (map (lambda (r) (circle (+ 60 (* r 50)) 100 (* r 7) :fill color)) radii))

; for-each：批量 add
(for-each (lambda (shape) (add shape)) circles)

; filter：条件筛选
(define even-data (filter even? data))

; reduce：汇总计算
(define total-area (reduce + 0 (map circle-area radii)))
```

---

## 五、颜色与样式操作

### 5.1 颜色构建
```scheme
(color/rgb 255 99 71)                      → "#FF6347"
(color/rgba 138 43 226 128)                → "#8B2BE280"
; 也支持 CSS 命名色、"#RRGGBB"、"#RRGGBBAA"、"rgba(r,g,b,a)"
```

### 5.2 Style 系统
```scheme
; 定义命名风格（可包含 roughness 等参数）
(define-style 'my-style :fill "#3498DB" :stroke "#2C3E50" :stroke-width 3 :roughness 1.5)

; 应用风格（两种方式）
(add (circle 100 100 50 :style 'my-style))      ; kwarg 方式
(add (use-style (circle 100 100 50) 'my-style))  ; 函数方式
```

### 5.3 Palette 系统
```scheme
; 预置调色板
(add (character :style 'cel :palette 'dark-hero))

; 自定义调色板
(define-palette 'my-pal :colors '("#FF6B6B" "#4ECDC4" "#45B7D1"))
(add (circle 100 100 30 :fill (palette-ref 'my-pal 0)))
```

---

## 六、常见构建模式速查

| 目标 | 模式 |
|------|------|
| 多个相同物体 | `map` + `lambda` 批量创建 + `for-each add` |
| 分层叠加 | 多个 `rect` 从远到近渐变填充 |
| 递归网格 | 双递归（行列迭代），所有状态通过参数传递 |
| 等距 3D 瓦片 | 左面/右面/顶面三个 `polygon` 拼成一个 `group` |
| 多通道导出 | `render-channels` 一次性输出 albedo + normal + specular |
| 精灵表单 | `render-spritesheet` 排列多个精灵并输出 JSON 元数据 |
| 多尺寸图标 | `render-set` 自动输出 1x/2x/4x |
| 图层滤镜 | `make-layer` + `:filter (drop-shadow ...)` + `composition-add` |
| 动画并行 | `parallel anim1 anim2 ...` + `play` |
| 自定义卷积 | `convolution :width 3 :height 3 :kernel '(list ...)` |
| 手绘效果 | `:roughness 2 :bowing 1 :fill-style "cross-hatch"` |

---

> 以下为完整内置函数参考（附录）

---

## 附录 A：特殊形式（Special Forms）

| 形式 | 签名 | 说明 |
|------|------|------|
| `define` | `(define name val)` / `(define (fn params ...) body ...)` | 绑定变量或定义函数 |
| `lambda` | `(lambda (params ...) body ...)` | 创建闭包，支持 `(lambda (a . rest) body)` 变参 |
| `if` | `(if cond then [else])` | 条件分支 |
| `cond` | `(cond (test expr ...) ... (else expr ...))` | 多分支条件 |
| `and` | `(and expr ...)` | 短路与 |
| `or` | `(or expr ...)` | 短路或 |
| `begin` | `(begin expr ...)` | 顺序求值 |
| `set!` | `(set! name val)` | 按词法作用域修改变量 |
| `let` | `(let ((var val) ...) body ...)` | 并行局部绑定 |
| `let*` | `(let* ((var val) ...) body ...)` | 串行局部绑定（常用，因 PML 的 let 是并行的） |
| `letrec` | `(letrec ((var val) ...) body ...)` | 递归局部绑定 |
| `quote` | `(quote expr)` / `'expr` | 阻止求值 |
| `quasiquote` | `` `expr `` | 准引用，配合 `,unquote` 插值 |
| `import` | `(import "path.pml" [as prefix])` | 加载模块 |
| `provide` | `(provide sym ...)` | 导出符号到模块环境 |
| `defmacro` | `(defmacro name (params ...) body ...)` | 卫生宏（变量自动重命名） |
| `macroexpand` | `(macroexpand expr)` | 展开宏（调试用） |
| `gensym` | `(gensym ["prefix"])` | 生成唯一符号 |
| `assert` | `(assert expr)` | 断言 |

**重要**：PML 的 `let` 是**并行绑定**（不能 `(let ((a 1) (b (+ a 1))) ...)` 依赖前一个变量）。
需要串行绑定请用 `let*`。

```scheme
; let — 并行（两个表达式在相同环境中求值）
(let ((x 5) (y 10)) (+ x y))  → 15

; let* — 串行（后面的表达式可以看到前面的绑定）
(let* ((x 5) (y (+ x 1))) y)  → 6
```

---

## 附录 B：算术与比较

| 函数 | 签名 | 说明 |
|------|------|------|
| `+` `-` `*` `/` | `(+ a b ...)` | 基本算术 |
| `quotient` `remainder` `modulo` | `(quotient a b)` | 整数除法 |
| `abs` `floor` `ceil` `round` `truncate` | `(abs n)` | 取整函数 |
| `sqrt` `expt` `exp` `log` | `(sqrt n)` | 数学函数 |
| `sin` `cos` `tan` `asin` `acos` `atan` | `(sin rad)` | 三角函数（弧度） |
| `pi` | `(pi)` | π 常量 |
| `random` | `(random [max])` | 随机数 |
| `=` `<` `>` `<=` `>=` | `(= a b ...)` | 比较 |
| `zero?` `positive?` `negative?` `even?` `odd?` `number?` | `(zero? n)` | 谓词 |

---

## 附录 C：集合与容器

### 列表
| 函数 | 签名 | 说明 |
|------|------|------|
| `cons` `car` `cdr` | `(cons a b)` | 基础 cons 操作 |
| `list` `length` `append` `reverse` | `(list a b ...)` | 列表构建 |
| `list-ref` `list-tail` | `(list-ref lst i)` | 索引访问 |
| `null?` `pair?` `list?` | `(null? v)` | 类型谓词 |
| `memq` `member` `assq` `assoc` | `(memq v lst)` | 搜索 |
| `map` `filter` `reduce` | `(map fn lst ...)` | 高阶函数 |
| `sort` | `(sort lst comp)` | 排序 |
| `range` | `(range start end)` | 创建整数范围列表 |
| `for-each` | `(for-each fn lst ...)` | 遍历执行（返回空列表） |
| `apply` | `(apply fn lst)` | 以列表为参数调用函数 |

### 向量
| 函数 | 签名 | 说明 |
|------|------|------|
| `make-vector` `vector-ref` `vector-set!` | `(make-vector n [init])` | 向量 CRUD |
| `vector-length` `vector->list` `list->vector` | `(vector-length v)` | 转换 |
| `vector?` | `(vector? v)` | 谓词 |

### 哈希表
| 函数 | 签名 | 说明 |
|------|------|------|
| `make-hash` `hash-ref` `hash-set!` `hash-delete!` | `(make-hash)` | 哈希表 CRUD |
| `hash-keys` `hash-values` | `(hash-keys h)` | 遍历 |
| `hash?` | `(hash? v)` | 谓词 |

### 字符串
| 函数 | 签名 | 说明 |
|------|------|------|
| `string-append` `string-length` `string-ref` `substring` | `(string-append a b)` | 字符串操作 |
| `string->number` `number->string` `symbol->string` `string->symbol` | `(string->number s)` | 类型转换 |
| `string?` `string=?` `string-ci=?` `string<?` | `(string? v)` | 字符串谓词 |
| `format` | `(format fmt ...)` | 格式化输出 |

### 通用谓词
| 函数 | 签名 | 说明 |
|------|------|------|
| `eq?` `eqv?` `equal?` | `(eq? a b)` | 相等比较（identity/value/structural） |
| `not` `boolean?` `symbol?` `keyword?` `procedure?` | `(not v)` | 逻辑与类型谓词 |
| `graphic-object?` `canvas?` `transform?` | `(graphic-object? v)` | PML 类型谓词 |

---

## 附录 D：图形形状（2D）

所有形状函数返回**不可变 GraphicObject**。使用 kwarg 传递样式参数。

### 通用 kwarg
`:fill`（填充色）、`:stroke`（描边色）、`:stroke-width`（描边宽度，默认 1.0）、
`:opacity`（不透明度 0~1）、`:blend-mode`（混合模式符号）、
`:roughness`（手绘抖动程度，>0 启用 rough）、`:bowing`（线条弯曲度）、
`:seed`（随机种子）、`:fill-style`（填充纹理）、`:style`（引用已定义 style）。

### 形状函数
| 函数 | 签名 | 说明 |
|------|------|------|
| `circle` | `(circle cx cy r :kw ...)` | 圆 |
| `rect` | `(rect x y w h :rx R :kw ...)` | 矩形，`:rx`=圆角半径 |
| `ellipse` | `(ellipse cx cy rx ry :kw ...)` | 椭圆 |
| `line` | `(line x1 y1 x2 y2 :kw ...)` | 线段 |
| `polygon` | `(polygon pts :kw ...)` | 多边形，pts=`(list (list x y) ...)` |
| `text` | `(text x y str :fill C :font-size N :font-family F)` | 文本（CJK 受限制） |
| `path` | `(path cmds :d SVG_STR :kw ...)` | SVG 路径 |
| `group` | `(group go ...)` | 组合多个形状 |
| `image` | `(image path :x X :y Y :width W :height H :crop CROP)` | 嵌入图片 |

### path 命令
`move-to` `line-to` `cubic-to` `quad-to` `arc` `close` 及各自 `-r` 相对版本。

或使用 SVG 字符串：`(path :d "M10 10 L100 50 C150 0 200 100 250 50 Z")`

### image 用法
```scheme
(image "assets/player.png" :x 100 :y 50)
(image "bg.jpg" :x 0 :y 0 :width 800 :height 600)
(define crop (list 10 10 32 32))
(image "tileset.png" :x 0 :y 0 :crop crop)
```

---

## 附录 E：变换

| 函数 | 签名 | 说明 |
|------|------|------|
| `translate` | `(translate x y)` | 平移变换矩阵 |
| `rotate` | `(rotate deg)` | 旋转（度） |
| `scale` | `(scale sx [sy])` | 缩放 |
| `shear` | `(shear shx shy)` | 斜切 |
| `compose` | `(compose t1 t2 ...)` | 合成多个变换 |
| `matrix-inverse` | `(matrix-inverse t)` | 逆矩阵 |
| `matrix-apply` | `(matrix-apply t x y)` | 应用变换到点 |

```scheme
; 应用到 GraphicObject
(with-transform go (compose (translate 100 0) (rotate 45) (scale 2 1)))
(translate-object go 50 50)
(rotate-object go 45)
(scale-object go 2 2)
```

---

## 附录 F：Canvas 与渲染

### Canvas 管理
| 函数 | 签名 | 说明 |
|------|------|------|
| `canvas` | `(canvas w h :bg "white")` | 创建并设置为当前 Canvas |
| `sprite-canvas` | `(sprite-canvas w h :bg "transparent" :anchor 'center-bottom)` | 精灵画布 |
| `clear-canvas` | `(clear-canvas)` | 清空当前 Canvas |
| `add` | `(add [canvas] go)` | 添加 GraphicObject 到 Canvas |

### 渲染
| 函数 | 签名 | 说明 |
|------|------|------|
| `render` | `(render "output.png" :fps 12)` | 渲染当前 Canvas 到文件 |
| `render-set` | `(render-set "prefix" :content go :scales '(1 2 4) :base-size '(64 64))` | 多尺寸导出 |
| `render-spritesheet` | `(render-spritesheet "sheet.png" go ... :cols N :cell-w N :cell-h N :padding N)` | 精灵表单输出 |
| `render-channels` | `(render-channels go :output "prefix" :channels '(albedo specular normal))` | 多通道渲染 |

### 后端管理
| 函数 | 签名 | 说明 |
|------|------|------|
| `set-backend!` | `(set-backend! "skia")` | 切换渲染后端 |
| `list-backends` | `(list-backends)` | 列出可用后端 |
| `backend-capabilities` | `(backend-capabilities)` | 当前后端能力 |

---

## 附录 G：动画

| 函数 | 签名 | 说明 |
|------|------|------|
| `animate` | `(animate go 'property from to duration :ease 'ease-in-out)` | 创建补间动画 |
| `play` | `(play [anim])` | 播放动画 |
| `stop` | `(stop [anim])` | 停止 |
| `pause` | `(pause [anim])` | 暂停 |
| `seek` | `(seek anim time)` | 跳转到指定时间 |
| `animation-state` | `(animation-state [anim])` | 查询状态 |
| `every-frame` | `(every-frame proc)` | 逐帧回调 |
| `parallel` | `(parallel anim ...)` | 并行动画组合 |
| `sequence` | `(sequence anim ...)` | 序列动画组合 |

**Easing 名称**：`"linear"` `"ease"` `"ease-in"` `"ease-out"` `"ease-in-out"` `"quad"` `"cubic"` `"quart"` `"quint"` `"sine"` `"exp"` `"circ"`

**可动画属性**：`'x` `'y` `'rotation` `'scale` `'fill` `'opacity` 等 GraphicObject 属性

---

## 附录 H：图层与滤镜

### 图层系统
| 函数 | 签名 | 说明 |
|------|------|------|
| `make-composition` | `(make-composition name w h :bg COLOR)` | 创建合成场景 |
| `make-layer` | `(make-layer name go :offset '(x y) :opacity F :visible BOOL :filter FILTER)` | 创建图层 |
| `composition-add` | `(composition-add comp layer ...)` | 添加图层到合成 |
| `composition-render` | `(composition-render comp "output.png")` | 渲染合成 |
| `composition-animate` | `(composition-animate comp "name" :frames N :fps N :cols N :step-fn FN)` | 合成场景逐帧动画 |

### 图层滤镜
| 函数 | 签名 | 说明 |
|------|------|------|
| `drop-shadow` | `(drop-shadow :dx F :dy F :blur F :color C)` | 投影 |
| `inner-shadow` | `(inner-shadow :dx F :dy F :blur F :color C)` | 内阴影 |
| `outer-glow` | `(outer-glow :blur F :color C)` | 外发光 |
| `inner-glow` | `(inner-glow :blur F :color C)` | 内发光 |
| `blur` | `(blur :radius F :type 'gaussian)` | 模糊 |
| `sharpen` | `(sharpen :amount F :radius F)` | 锐化 |
| `edge-detect` | `(edge-detect :type 'sobel)` | 边缘检测 |
| `emboss` | `(emboss :direction F)` | 浮雕 |
| `convolution` | `(convolution :width N :height N :kernel '(vals ...))` | 自定义卷积 |
| `color-adjust` | `(color-adjust :brightness F :contrast F :saturation F :hue F)` | 综合颜色调整 |
| `levels` | `(levels :in-low F :gamma F :in-high F)` | 色阶 |
| `curves` | `(curves :channel 'rgb :points '((x y) ...))` | 曲线调整 |
| `threshold` | `(threshold :value N)` | 二值化 |
| `posterize` | `(posterize :levels N)` | 色调分离 |
| `filter-chain` | `(filter-chain filter ...)` | 滤镜串联 |
| `apply-filter` | `(apply-filter filter go)` | 应用滤镜到对象 |

### 混合模式
`'normal` `'multiply` `'screen` `'overlay` `'soft-light` `'hard-light` `'darken` `'lighten`
`'color-dodge` `'color-burn` `'difference` `'exclusion` `'hue` `'saturation` `'color` `'luminosity` `'plus`

---

## 附录 I：Tilemap 系统

| 函数 | 签名 | 说明 |
|------|------|------|
| `define-tileset` | `(define-tileset name :tile-size N :tiles '((id name graphic) ...))` | 定义瓦片集 |
| `make-tilemap` | `(make-tilemap tileset cols rows :projection 'orthogonal :layers 1)` | 创建瓦片地图 |
| `tilemap-set!` | `(tilemap-set! tilemap layer col row tile-id)` | 设置瓦片 |
| `render-tilemap` | `(render-tilemap tilemap :output "file.png" :bg COLOR)` | 渲染瓦片地图 |

**投影类型**：`'orthogonal`（正交）/ `'isometric`（等距，自动 Painter's Algorithm 深度排序）

```scheme
(define-tileset 'terrain :tile-size 32
  :tiles '((1 grass  (rect 0 0 32 32 :fill "#2ecc71"))
           (2 stone  (rect 0 0 32 32 :fill "#95a5a6"))))

(define tm (make-tilemap 'terrain 12 10 :projection 'orthogonal :layers 2))
(tilemap-set! tm 0 1 0 1)  ; layer=0, col=1, row=0 → tile-id=1 (grass)
```

---

## 附录 J：着色器与噪声

| 函数 | 签名 | 说明 |
|------|------|------|
| `shader` | `(shader "sksl-source" [:name "name"])` | 编译 SkSL 源码，返回 handle |
| `apply-shader!` | `(apply-shader! go shader-handle)` | 应用着色器到 GraphicObject |
| `make-uniforms` | `(make-uniforms val1 val2 ...)` | 创建 uniform 数据 |
| `apply-uniforms` | `(apply-uniforms shader-handle uniform-data)` | 绑定 uniform 数据 |
| `uniform-float` | `(uniform-float shader-handle name val)` | 快捷设置单个 float uniform |
| `bind-textures` | `(bind-textures shader-handle :textures '((slot go) ...))` | 多纹理绑定 |

### 噪声着色器
| 函数 | 签名 | 说明 |
|------|------|------|
| `noise-fractal` | `(noise-fractal [:freq-x 0.02 :freq-y 0.02 :octaves 4 :seed 0 :tile-width 0 :tile-height 0 :lacunarity 2.0 :persistence 0.5])` | 分形布朗运动噪声 |
| `noise-turbulence` | 同 `noise-fractal` 参数 | 湍流噪声（绝对值变体） |
| `noise-voronoi` | `(noise-voronoi :cell-size 32 :seed 0 :jitter 0.5)` | Voronoi 细胞噪声 |
| `noise-warp` | `(noise-warp base warp :amount 20.0 :freq 0.008)` | 域扭曲 |
| `noise-blend` | `(noise-blend a b :mode 'gradient :direction 'vertical :weight F)` | 噪声混合过渡 |
| `quantize-noise` | `(quantize-noise noise :levels '((threshold "color") ...))` | 噪声量化为离散色阶 |

---

## 附录 K：Sprite 系统

### Style
| 函数 | 签名 | 说明 |
|------|------|------|
| `define-style` | `(define-style name :fill C :stroke C :stroke-width W :roughness R)` | 定义命名风格 |
| `use-style` | `(use-style go name)` | 应用风格到对象 |

### Palette
| 函数 | 签名 | 说明 |
|------|------|------|
| `define-palette` | `(define-palette name :colors '(c1 c2 c3 ...))` | 定义调色板 |
| `palette-ref` | `(palette-ref name [index])` | 取调色板颜色 |

预置调色板：`pico-8` `nes` `c64` `gameboy` `sega` `snes` `arctic16` `lava16` `forest16` `ocean16` `warm-skin` `dark-hero`

### 组件
`character` `body` `head` `eyes` `mouth` `hair` `outfit` `weapon` `potion` `chest` `generic-item`
`button` `panel` `health-bar` `icon` `tile` `decoration` `background`

```scheme
; 自定义角色
(define hero (character
  :head (head :shape 'oval :skin "#F5D0A9")
  :eyes (anime-eyes :style 'shoujo :color "#4A90D9")
  :mouth (mouth :style 'smile)
  :hair (hair :style 'long :color "#8E44AD")
  :body (body :height 170 :build 'slim)
  :outfit (outfit :top 'hoodie :color-top "#E74C3C")
  :style 'cel :scale 1.2))

; UI 组件
(button :label "START" :width 140 :height 40 :style 'rounded :color "#27AE60")
(panel :width 420 :height 280 :style 'simple :color "#34495E")
(health-bar :value 0.72 :width 200 :height 20 :style 'gradient :color "#E74C3C")
(icon :type 'heart :size 32 :color "#E74C3C")
```

---

## 附录 L：3D 图形

| 函数 | 签名 | 说明 |
|------|------|------|
| `cube3d` | `(cube3d size :fill C :stroke C)` | 立方体 |
| `cuboid3d` | `(cuboid3d w h d :fill C)` | 长方体 |
| `rounded-cuboid3d` | `(rounded-cuboid3d w h d r :fill C)` | 圆角长方体 |
| `cone3d` | `(cone3d radius height :fill C)` | 圆锥 |
| `plane3d` | `(plane3d w h :fill C)` | 平面 |
| `sphere3d` | `(sphere3d radius :fill C)` | 球体 |
| `rotate-x` `rotate-y` `rotate-z` | `(rotate-x deg)` | 3D 旋转 |
| `translate3d` | `(translate3d x y z)` | 3D 平移 |
| `scale3d` | `(scale3d sx sy sz)` | 3D 缩放 |
| `camera` | `(camera :position '(x y z) :target '(x y z) :fov 45 :projection 'perspective)` | 3D 相机 |

```scheme
(camera :position '(4 3 5) :target '(0 0 0) :projection 'perspective)
(define scene (group (cube3d 1.5 :fill "#e74c3c") (sphere3d 0.8 :fill "#3498db")))
(add scene)
```

---

## 附录 M：骨骼与 IK

| 函数 | 签名 | 说明 |
|------|------|------|
| `defskeleton` | `(defskeleton name (x y) (joint name :pos (dx dy) :length L :angle A :min MIN :max MAX) ...)` | 定义骨骼 |
| `instantiate-skeleton` | `(instantiate-skeleton name x y)` | 实例化 |
| `joint-position` | `(joint-position instance joint-name)` | 获取关节位置 |
| `bind-skin` | `(bind-skin go skeleton joint-name ...)` | 绑定骨骼到皮肤 |
| `ik-solve` | `(ik-solve skel end-effector tx ty :max-iterations 20 :tolerance 0.5)` | IK 求解（FABRIK） |

---

## 附录 N：模块系统

```scheme
(import "path.pml")           → 加载，前缀=文件名（去掉后缀）
(import "path.pml" as pref)   → 加载，指定前缀
(provide sym1 sym2)           → 显式导出符号
pref/sym                      → 通过前缀访问模块导出
```

**查找顺序**：绝对路径 → 原路径+.pml → 相对于引用文件目录 → 同上+.pml

**循环依赖检测**：自动检测并报错

---

## 附录 O：手绘风格（Rough.js）

通过 `:roughness` 参数启用（`roughness > 0`）：

| 参数 | 默认 | 说明 |
|------|------|------|
| `:roughness` | 0 | 抖动程度（推荐 1~3） |
| `:bowing` | 1.0 | 线条弯曲度 |
| `:seed` | 0 | 随机种子 |
| `:fill-style` | `"solid"` | `"cross-hatch"` `"dots"` `"zigzag"` `"dashed"` |

```scheme
(circle 100 100 50 :fill "#FF6B6B" :roughness 2 :bowing 1)
(rect 50 50 200 150 :fill "#51CF66" :roughness 2 :fill-style "cross-hatch")
```

---

## 附录 P：CLI 命令

| 命令 | 说明 |
|------|------|
| `pml file.pml` | 执行 PML 文件 |
| `pml file.pml -o ./out` | 指定输出目录 |
| `pml file.pml --json` | JSON 格式输出 |
| `pml file.pml --watch` | 监听文件变更自动重渲染 |
| `pml` | 交互式 REPL |
| `pml-mcp` | MCP JSON-RPC 服务（stdio） |

---

## 附录 Q：常见陷阱

1. **不要忘记 `set-backend!`** — 必须在 `canvas` 之前调用
2. **`let` 是并行绑定的** — 需要顺序赋值时用 `let*`
3. **所有 GraphicObject 不可变** — `with-transform` 等不修改原对象，返回新对象
4. **`add` 顺序 = z-order** — 后添加的图形盖在先添加的上面
5. **`car`/`cdr` 处理列表，不支持 `cadr`** — 用 `car` + `cdr` 组合或 `list-ref`
6. **CJK 文本受限** — 当前 Skia 后端没有 font fallback，中文字符可能显示为方块
7. **`render` 输出格式由文件名后缀决定** — `.png`/`.gif`
8. **模块文件路径相对于当前文件解析** — 非相对于工作目录
9. **不要在列表内置 `nil` 混用** — 使用空列表 `'()` 表示空
10. **递归深度受限** — 大数据集用 `map`/`for-each` 替代手动递归

---

## 附录 R：错误处理

PML C++ 实现的错误处理遵循 **无异常控制流** 原则：所有运行时错误通过返回值而不是 `throw` 传递。

### 错误类型

| 错误 | 工厂函数 | 触发场景 |
|------|----------|----------|
| 类型错误 | `type_error(msg)` / `type_error(loc, msg)` | 参数类型不符合预期（`+` 期待数字、`hash-ref` 期待 hash 表等） |
| 语法错误 | `syntax_error(msg)` / `syntax_error(loc, msg)` | SVG 路径解析失败、语法结构错误 |
| 未绑定变量 | `unbound_error(loc, name)` | 引用了未定义的变量或符号 |
| 参数数量错误 | `arity_error(loc, expected, got)` | 内置函数参数数量不匹配 |
| 导入错误 | `import_error(loc, msg)` | 模块文件未找到或无法加载 |
| 循环导入 | `circular_import_error(name)` | 模块之间存在循环依赖 |

`type_error` 和 `syntax_error` 各有两个重载：一个接受 `SourceLocation`（提供行号信息），一个只接受字符串消息。

### 返回值模式

```cpp
// 类型错误
return std::unexpected(type_error("+ expected numeric arguments"));

// 带位置信息的参数数量错误
return std::unexpected(arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));

// 字符串错误（不带位置）
return std::unexpected(type_error("set-backend!: argument must be a string or symbol"));
```

### 各模块错误场景

#### 算术与比较（builtins_arithmetic.cpp）
所有算术运算符和比较函数都会检查参数类型：
- `+` `-` `*` `/` `%` `modulo` `remainder` `quotient` — 期待数字参数
- `abs` `floor` `ceil` `round` `truncate` `sqrt` `expt` — 期待数字参数
- `even?` `odd?` `zero?` `positive?` `negative?` — 期待数字参数
- `sin` `cos` `tan` `asin` `acos` `atan` — 期待数字（弧度）

#### 容器（builtins_containers.cpp）
- `make-hash` — 参数必须为偶数个（key-value 对）
- `hash-ref` / `hash-set!` / `hash-delete!` — 第一个参数必须是 hash 表，key 必须是字符串
- `make-vector` — 参数必须合法
- `vector-ref` / `vector-set!` — 第一个参数必须是 vector
- `list->vector` — 参数必须是 list
- `vector-fill!` / `vector-copy` — 类型检查、索引范围检查

#### 图形与渲染（graphics/render.cpp）
- SVG path 解析 — `syntax_error("invalid SVG path command at position N")`
- `render-set` — `:content` 必须是 GraphicObject
- `render-tilemap` — 第一个参数必须是字符串/符号，`:output` 必须指定
- 引用的 tilemap/tileset 不存在时报错

#### 动画（animation/timeline.cpp）
- `animate` — 第一个参数必须是 GraphicObject，property 必须是符号/字符串，from/to 必须为数字或颜色字符串
- `seek` — time 参数必须为数字
- `every-frame` — 参数必须为可调用过程

#### Sprite 系统（sprites/style.cpp, palette.cpp）
- `define-style` / `use-style` — 名称必须是符号或字符串
- `define-palette` — 名称必须是字符串，颜色必须是列表

#### 图层与滤镜（layer/）
- `make-layer` / `make-group` — 名称必须是字符串
- `composition-add` / `layer-with` — 类型检查
- `composition-render` / `composition-animate` — 参数类型校验

#### 着色器（shader_builtins.cpp）
- SkSL 编译失败时返回 `type_error`，包含 Skia 编译错误信息

#### 后端（backend_builtins.cpp）
- `set-backend!` — 参数必须是字符串或符号
- `backend-available?` — 参数必须是字符串或符号


