---
name: pml-image
description: 用 PML（Picture Markup Language）生成图片/动画/纹理。当用户要求"画/绘制/生成一张图"、"render/draw an image"、"PML 代码"、"纹理映射/texture/tilemap/瓦片地图"时自动触发。
metadata:
  type: skill
  trigger: 画|绘制|生成.*图|render|draw.*image|PML|纹理|texture|uv mapping|tilemap|瓦片地图
---

# PML 图片生成技能

## 概述

PML（Picture Markup Language）是一个 Lisp 方言 DSL，用于代码到图像的生成。
源文件后缀为 `.pml`，通过 CLI 工具编译渲染为 PNG/GIF。

## 快速开始

### 运行方式

```bash
G:/Project/PML/build/debug/src/pml/cli/Debug/pml.exe 路径/到/文件.pml
```
**
CWD 设为 `.pml` 文件所在目录。渲染的图片输出在 `.pml` 文件同目录下。（模块化项目中 `import` 路径相对于入口文件目录。）**

### 最小示例

```pml
;; 保存为 hello.pml
(set-backend! "skia")
(canvas 200 200 :bg "#ffffff")
(add (circle 100 100 80 :fill "#ff0000"))
(render "hello.png")
```

运行：`pml.exe hello.pml` → 生成 `hello.png`

### 项目组织（模块化）

PML 支持模块化开发，通过 `import`/`provide` 组织代码：

```
my-project/
  main.pml       # 入口文件，import 子模块，组装并渲染
  shapes.pml     # 定义图形函数，provide 导出
  textures.pml   # 定义纹理，provide 导出
```

**main.pml**
```pml
(import "shapes" :prefix s)
(import "textures" :prefix t)

(set-backend! "skia")
(canvas 400 400 :bg "#1a1a2e")
(add (s/star 200 200 80 :fill "#ffd700"))
(add (t/apply-tex (rect 50 50 100 100)))
(render "output.png")
```

**shapes.pml**
**shapes.pml**
```pml
(provide rounded-box)

(defn rounded-box (x y w h &kwargs)
  "Draw a rounded rectangle with configurable corner radius"
  (rect x y w h :rx (or (kw_get kwargs :rx) 8)
        :fill (or (kw_get kwargs :fill) "#74b9ff")
        :stroke (or (kw_get kwargs :stroke) "#0984e3")
        :stroke-width (or (kw_get kwargs :stroke-width) 2)))
```

(define-texture checker (64 64)
  (group ...))

(defn apply-tex (shape)
  (texture-map shape :source checker :mode :planar))
```

---

# API 参考

---

## 1. 核心语法

### 1.1 基础

PML 使用 S-expression 语法。`;` 开头为行注释。

```pml
42          ;; 整数
3.14        ;; 浮点数
"hello"     ;; 字符串
true        ;; 布尔真（#t 也可）
false       ;; 布尔假（#f 也可）
nil         ;; 空值
sym-name    ;; 符号
:keyword    ;; 关键字
```

### 1.2 变量定义

| 函数 | 签名 | 说明 |
|------|------|------|
| `define` | `(define name value)` | 定义全局变量 |
| `let` | `(let ((name expr) ...) body)` | 串行绑定 |
| `let-par` | `(let-par ((name expr) ...) body)` | 并行绑定 |
| `let*` | `(let* ...)` | `let` 的别名 |
| `set!` | `(set! name value)` | 修改已绑定的变量 |

```pml
(define pi 3.14159)
(let ((x 10) (y 20))
  (+ x y))
```

### 1.3 算术运算

| 函数 | 签名 | 说明 |
|------|------|------|
| `+` | `(+ a b ...)` | 加法 |
| `-` | `(- a b ...)` | 减法 |
| `*` | `(* a b ...)` | 乘法 |
| `/` | `(/ a b ...)` | 除法 |
| `mod` | `(mod a b)` | 取模 |
| `abs` | `(abs a)` | 绝对值 |
| `round` | `(round a)` | 四舍五入 |
| `floor` | `(floor a)` | 向下取整 |
| `ceil` | `(ceil a)` | 向上取整 |
| `sqrt` | `(sqrt a)` | 平方根 |
| `sin` | `(sin a)` | 正弦（弧度） |
| `cos` | `(cos a)` | 余弦（弧度） |
| `random` | `(random)` | 返回 [0, 1) 随机数 |
| `random-seed!` | `(random-seed! seed)` | 设置随机种子 |

### 1.4 比较与逻辑

| 函数 | 说明 |
|------|------|
| `(= a b)` `(< a b)` `(> a b)` `(<= a b)` `(>= a b)` | 数值比较 |
| `(eq? a b)` | 引用相等 |
| `(equal? a b)` | 结构相等 |
| `(and a b ...)` `(or a b ...)` `(not a)` | 逻辑运算 |

### 1.5 条件

```pml
(if condition then-expr else-expr)
(cond
  (condition1 expr1)
  (condition2 expr2)
  (else expr-default))
(when condition body...)
(unless condition body...)
(case key
  ((value ...) expr...)
  (else expr...))
```

### 1.6 函数

```pml
;; 命名函数
(defn add-one (x) (+ x 1))

;; 匿名函数
(lambda (x) (* x x))

;; 关键字参数
(defn draw-box (x y w h &kwargs)
  ;; 使用 kw_string/kw_double 解析 :fill、:stroke 等
  ...)

;; 可选参数
(defn greet ((name "world"))
  (string-append "Hello, " name))
```

### 1.7 列表操作

| 函数 | 说明 |
|------|------|
| `(list a b ...)` | 创建列表 |
| `(car lst)` | 取第一个元素 |
| `(cons a lst)` | 头部插入 |
| `(length lst)` | 长度 |
| `(map fn lst)` | 映射 |
| `(filter fn lst)` | 过滤 |
| `(reduce fn init lst)` | 归约 |
| `(range end)` | 生成数列 |
| `(enumerate lst)` | 带索引枚举 |
| `(zip a b)` | 两个列表压缩 |

### 1.8 宏

```pml
(defmacro repeat (n &body body)
  `(dotimes (i ,n) ,@body))
```

### 1.9 字符串操作

`string-append` `string-length` `string-split` `string-join` `number->string` `string->number`

### 1.10 类型判断

`nil?` `int?` `float?` `number?` `string?` `bool?` `symbol?` `list?` `hash?` `vector?` `proc?` `typeof`

---

## 2. 图形基元

### `rect`
**签名**: `(rect x y w h [:fill color] [:stroke color] [:stroke-width w] [:rx r])`

绘制矩形。`rx` 为圆角半径。

```pml
(add (rect 50 50 100 100 :fill "#ff0000" :rx 8))
```

### `circle`
**签名**: `(circle cx cy r [:fill color] [:stroke color] [:stroke-width w])`

```pml
(add (circle 100 100 50 :fill "#00ff00" :stroke "#000" :stroke-width 2))
```

### `ellipse`
**签名**: `(ellipse cx cy rx ry [:fill color] [:stroke color] [:stroke-width w])`

```pml
(add (ellipse 100 100 80 40 :fill "#0000ff"))
```

### `line`
**签名**: `(line x1 y1 x2 y2 [:stroke color] [:stroke-width w])`

```pml
(add (line 10 10 190 190 :stroke "#ff00ff" :stroke-width 3))
```

### `polygon`
**签名**: `(polygon points [:fill color] [:stroke color] [:stroke-width w])`

`points` 为坐标对列表：`((x1 y1) (x2 y2) ...)` 或平铺的坐标列表 `(x1 y1 x2 y2 ...)`

```pml
(add (polygon (list (list 100 20) (list 180 140) (list 20 140))
      :fill "#ff8800" :stroke "#000"))
```

### `path`
**签名**: `(path [:d svg-path-string] [:fill color] [:stroke color] [:stroke-width w])`

```pml
(add (path :d "M10 10 C 20 20, 40 20, 50 10" :stroke "#333" :stroke-width 2))
```

### `text`
**签名**: `(text x y content [:fill color] [:font-size size] [:align 'left|'center|'right])`

```pml
(add (text 200 50 "Hello PML" :fill "#fff" :font-size 24 :align 'center))
```

### `group`
**签名**: `(group children...)`

```pml
(add (group
  (circle 50 50 30 :fill "#f00")
  (rect 20 80 60 20 :fill "#00f")))
```

### 综合示例：图形基元

```pml
(set-backend! "skia")
(canvas 400 300 :bg "#2d3436")

(add (rect   30  30 80 80 :fill "#e17055" :rx 8))
(add (circle 190  70 40 :fill "#00cec9"))
(add (ellipse 320 70 50 30 :fill "#6c5ce7"))
(add (line 30 200 370 200 :stroke "#dfe6e9" :stroke-width 2))
(add (polygon (list (list 200 250) (list 280 290) (list 120 290))
      :fill "#fdcb6e" :stroke "#e17055"))
(add (text 200 280 "Shapes Demo" :fill "#fff" :font-size 14 :align 'center))

(render "shapes.png")
```

---

## 3. 样式与颜色

### 颜色格式

```pml
"#ff0000"      ;; 16 进制 RGB
"#ff000080"    ;; 16 进制 RGBA（带 alpha）
"red"          ;; 命名颜色
"transparent"  ;; 透明
```

### 颜色函数

| 函数 | 说明 |
|------|------|
| `(rgb r g b)` | 0-255 整数分量创建颜色 |
| `(rgba r g b a)` | a 为 0-255 整数或 0.0-1.0 浮点数 |
| `(hex "#RRGGBB")` | 解析颜色字符串 |
| `(lerp-color c1 c2 t)` | 颜色插值 |

### 样式参数

所有形状共用以下关键字参数：

| 参数 | 说明 |
|------|------|
| `:fill color` | 填充色 |
| `:stroke color` | 描边色 |
| `:stroke-width n` | 线宽 |
| `:opacity n` | 0.0-1.0 透明度 |
| `:blend-mode mode` | 混合模式（见下） |

### 混合模式

`(blend-mode mode-string)` 设置混合模式：

`normal` `multiply` `screen` `overlay` `darken` `lighten` `color-dodge`
`color-burn` `hard-light` `soft-light` `difference` `exclusion` `hue`
`saturation` `color` `luminosity`

```pml
(add (rect 50 50 100 100 :fill "#ff0000" :blend-mode (blend-mode "multiply")))
```

### 综合示例：样式

```pml
(set-backend! "skia")
(canvas 300 300 :bg "#ffffff")

(add (circle 100 100 80 :fill (rgba 255 0 0 100)))
(add (circle 150 100 80 :fill (rgba 0 0 255 100)))
(add (circle 125 150 80 :fill (rgba 0 255 0 100)))

(add (text 150 270 "Blend Demo" :fill "#333" :font-size 14 :align 'center))
(render "blend.png")
```

---

## 4. 变换

| 函数 | 说明 |
|------|------|
| `(translate x y)` | 创建平移矩阵 |
| `(rotate deg)` | 创建旋转矩阵（角度制） |
| `(scale x y)` | 创建缩放矩阵 |
| `(compose . transforms)` | 组合多个变换矩阵 |

### `with-transform`
**签名**: `(with-transform shape :transform matrix)`

```pml
(add (with-transform (rect -50 -50 100 100 :fill "#e17055")
       :transform (compose (translate 200 150) (rotate 45))))
```

### `translate-object` / `rotate-object` / `scale-object`

```pml
(add (rotate-object (rect 50 50 100 100 :fill "#00cec9") 30))
(add (translate-object (circle 0 0 40 :fill "#ff0000") 100 100))
```

### 综合示例：变换

```pml
(set-backend! "skia")
(canvas 400 400 :bg "#1a1a2e")

(defn draw-snowflake (cx cy r n)
  (dotimes (i n)
    (let ((angle (* i (/ 360 n))))
      (add (with-transform (line 0 0 0 (- r) :stroke "#ffffff" :stroke-width 1.5)
             :transform (compose (translate cx cy) (rotate angle)))))))

(draw-snowflake 200 200 80 12)
(render "snowflake.png")
```

---

## 5. Canvas 与渲染

### `set-backend!`
**签名**: `(set-backend! "skia")`

设置渲染后端。必须在使用 `canvas` 之前调用。

### `canvas`
**签名**: `(canvas w h [:bg color])`

创建画布。`:bg` 设置背景色。

### `sprite-canvas`
**签名**: `(sprite-canvas w h [:bg color] [:anchor anchor] [:padding n])`

创建精灵画布（用于导出到 spritesheet）。

### `add`
**签名**: `(add graphic-object)`

将图形对象添加到当前画布。后添加的在上层（z-order）。

### `render`
**签名**: `(render filename)`

渲染当前画布为图片。输出格式由文件扩展名决定（`.png` / `.gif`）。

```pml
(set-backend! "skia")
(canvas 400 400 :bg "#16213e")
(add (circle 200 200 100 :fill "#e94560"))
(render "output.png")
```

### `render-set`
**签名**: `(render-set name graphic-object :scales (1 2 4) :base-width w :base-height h)`

渲染多分辨率图片集。

```pml
(render-set "icon" (circle 16 16 16 :fill "#e94560")
  :scales '(1 2 4) :base-width 32 :base-height 32)
```

### `render-spritesheet`
**签名**: `(render-spritesheet filename graphic-objects... [:cols n] [:cell-w w] [:cell-h h] [:padding n] [:bg color])`

```pml
(render-spritesheet "tiles.png"
  (rect 0 0 32 32 :fill "#ff0000")
  (rect 0 0 32 32 :fill "#00ff00")
  (rect 0 0 32 32 :fill "#0000ff")
  :cols 3 :cell-w 32 :cell-h 32)
```

### 综合示例：合成

```pml
(set-backend! "skia")
(canvas 300 300 :bg "#2d3436")

(add (rect 0 0 300 300 :fill "#0984e3" :opacity 0.3))
(add (circle 250 50 40 :fill "#fdcb6e"))
(add (rect 0 200 300 100 :fill "#00b894"))
(add (rect 80 120 100 120 :fill "#dfe6e9"))
(add (polygon (list (list 60 120) (list 130 70) (list 200 120)) :fill "#e17055"))
(add (rect 120 180 20 60 :fill "#636e72"))
(add (text 150 285 "My House" :fill "#fff" :font-size 12 :align 'center))
(render "house.png")
```

---

## 6. 纹理与 UV 映射

### `define-texture`
**签名**: `(define-texture name (width height) graphic-object)`

定义一个可复用的纹理。

```pml
(define-texture checker (64 64)
  (group
    (rect 0  0 32 32 :fill "#e94560")
    (rect 32 0 32 32 :fill "#16213e")
    (rect 0 32 32 32 :fill "#16213e")
    (rect 32 32 32 32 :fill "#e94560")))
```

### `texture-map`
**签名**: `(texture-map shape :source texture [:mode mode] [:wrap-x mode] [:wrap-y mode] [:filter mode])`

**参数**：
- `:source` — 纹理名称
- `:mode` — UV 模式：`:planar`（平面）、`:harmonic`（调和，默认）、`:explicit`（显式）
- `:wrap-x` / `:wrap-y` — 包裹模式：`:clamp`（默认）、`:repeat`、`:mirror`
- `:filter` — 过滤模式：`:linear`（默认）、`:nearest`
- `:uv-vertices` — 显式 UV 坐标（仅 `:explicit` 模式）

```pml
(add (texture-map (rect 40 40 320 160 :rx 8)
      :source checker :wrap-x :repeat :mode :planar))
```

### 纹理辅助函数

| 函数 | 说明 |
|------|------|
| `(texture? v)` | 判断是否为纹理值 |
| `(texture-width tex)` | 纹理宽度 |
| `(texture-height tex)` | 纹理高度 |
| `(texture-id tex)` | 纹理稳定 ID（缓存用） |
| `(texture-wrap! tex :x :repeat :y :clamp)` | 修改包裹模式 |
| `(texture-filter! tex :mode :nearest)` | 修改过滤模式 |

### 综合示例：纹理

```pml
(set-backend! "skia")
(canvas 400 400 :bg "#1a1a2e")

(define-texture starry (128 128)
  (group
    (rect 0 0 128 128 :fill "#0f0c29")
    (rect 0 0 128 64 :fill "#302b63" :opacity 0.5)
    (circle 96 32 16 :fill "#ffe066")
    (circle 16 24 1.5 :fill "#ffffff")
    (circle 64 20 2   :fill "#ffffff")
    (circle 24 64 1.5 :fill "#ffffff")))

(add (texture-map (rect 40 40 320 320 :rx 24)
      :source starry :mode :harmonic))
(add (text 200 380 "Starry Night" :fill "#ffffff" :font-size 14 :align 'center))

(render "texture_demo.png")
```

---

## 7. Rough 风格（手绘效果）

形状名前加 `rough_` 前缀：

| 函数 | 说明 |
|------|------|
| `(rough_circle cx cy r ...)` | 手绘圆 |
| `(rough_rect x y w h ...)` | 手绘矩形 |
| `(rough_ellipse cx cy rx ry ...)` | 手绘椭圆 |
| `(rough_line x1 y1 x2 y2 ...)` | 手绘线 |
| `(rough_polygon points ...)` | 手绘多边形 |
| `(rough_path ...)` | 手绘路径 |

**额外参数**：`:roughness n`（0-2，默认 1）`:bowing n`（0-2，默认 0.5）`:seed n`

```pml
(add (rough_circle 100 100 60 :fill "#e17055" :roughness 1.5 :bowing 0.8 :seed 42))
```

### 综合示例

```pml
(set-backend! "skia")
(canvas 400 300 :bg "#fff9ef")

(add (rough_rect 30 30 140 100 :fill "#74b9ff" :roughness 0.5 :seed 1))
(add (rough_circle 270 80 50 :fill "#ff7675" :roughness 1.5 :bowing 0.3 :seed 2))
(add (rough_line 60 200 340 220 :stroke "#636e72" :stroke-width 2 :roughness 1 :seed 3))
(add (rough_polygon (list (list 200 250) (list 280 280) (list 120 280))
      :fill "#fdcb6e" :roughness 2 :seed 4))

(render "rough_demo.png")
```

---

## 8. 边缘扰动

### `perturb-polygon`
**签名**: `(perturb-polygon points [:edge-noise n] [:edge-subdiv n] [:corner-radius n] [:seed n])`

```pml
(add (perturb-polygon '((0 0) (200 0) (200 200) (0 200))
      :edge-noise 15.0 :edge-subdiv 4 :seed 42))
```

---

## 9. 动画

### `animate`
**签名**: `(animate graphic keyframes...)`

```pml
(animate (circle 100 100 30 :fill "#e94560")
  (0   :translate 0   0)
  (0.5 :translate 200 0 :ease "ease-in-out")
  (1.0 :translate 0   0))
```

### 综合示例

```pml
(set-backend! "skia")
(canvas 300 300 :bg "#1a1a2e")

(define my-anim
  (animate (circle 150 50 30 :fill "#e94560")
    (0.0 :translate 0 0)
    (0.3 :translate 100 150)
    (0.6 :translate -100 150)
    (1.0 :translate 0 0)))

(render "animation.gif")
```

---

## 10. 图层合成

### `layer`
**签名**: `(layer name width height [:bg color])`

### `composition`
**签名**: `(composition width height layers...)`

```pml
(let ((bg (layer "bg" 200 200)))
  (layer-add bg (rect 0 0 200 200 :fill "#0984e3")))

(let ((fg (layer "fg" 200 200)))
  (layer-add fg (circle 100 100 50 :fill "#fdcb6e")))

(add (composition 200 200 bg fg))
```

---

## 11. 图像滤镜

| 函数 | 说明 |
|------|------|
| `(blur radius)` | 高斯模糊 |
| `(glow color radius)` | 发光 |
| `(drop-shadow dx dy blur color)` | 投影 |
| `(color-filter matrix)` | 颜色矩阵 |
| `(brightness n)` | 亮度调整 |
| `(contrast n)` | 对比度调整 |
| `(saturation n)` | 饱和度调整 |
| `(edge-detect)` | 边缘检测 |
| `(convolution matrix)` | 自定义卷积 |

`(add (apply-filter shape filter))` 将滤镜应用到形状。

```pml
(add (apply-filter (circle 100 100 80 :fill "#e94560") (blur 5)))
```

---

## 12. 精灵与调色板

### `define-style`
**签名**: `(define-style name attrs)`

```pml
(define-style hero
  '((fill . "#e94560") (stroke . "#16213e") (stroke-width . 2)))
```

### `define-palette`
**签名**: `(define-palette name colors)`

```pml
(define-palette ocean
  '((deep . "#0f0c29") (mid . "#302b63") (light . "#24243e")))
```

### `apply-style`
**签名**: `(apply-style shape style-name)`

```pml
(add (apply-style (circle 100 100 50) 'hero))
```

---

## 13. 瓦片地图

### `tilemap`
**签名**: `(tilemap tileset tile-data [:pos x y] [:tile-w w] [:tile-h h])`

### `isometric-tilemap`
**签名**: `(isometric-tilemap tileset tile-data [:tile-w w] [:tile-h h])`

```pml
(define tiles (tileset "tiles.png" :tile-w 32 :tile-h 32))
(add (tilemap tiles '((1 2 1) (2 0 2) (1 2 1)) :tile-w 32 :tile-h 32))
```

---

## 14. 3D 图形

### `mesh3d`
**签名**: `(mesh3d vertices faces)`

### `camera3d`
**签名**: `(camera3d [:fov n] [:near n] [:far n])`

```pml
(define cam (camera3d :fov 60))
(add (mesh3d vertices faces))
```

---

## 15. 模块系统

### `provide`
**签名**: `(provide symbol ...)`

```pml
(provide my-func my-var)
```

### `import`
**签名**: `(import "path" [:prefix prefix])`

```pml
(import "utils" :prefix u)
(u/helper-fn 42)
```

### 完整模块示例

**math.pml**
```pml
(provide clamp lerp)

(defn clamp (val min max)
  (cond ((< val min) min) ((> val max) max) (else val)))

(defn lerp (a b t)
  (+ a (* (- b a) t)))
```

**main.pml**
```pml
(import "math" :prefix m)

(set-backend! "skia")
(canvas 200 200 :bg "#ffffff")

(let ((x (m/clamp 150 0 100)))
  (add (circle x 100 30 :fill "#e94560")))

(render "output.png")
```

---

## 16. 综合项目示例

### 项目结构

```
game-assets/
  main.pml
  ui.pml
  sprites.pml
  environment.pml
```

**main.pml**
```pml
(import "sprites" :prefix s)
(import "environment" :prefix e)
(import "ui" :prefix ui)

(set-backend! "skia")
(canvas 800 600 :bg "#87ceeb")

(e/draw-ground 0 400 800 200)
(e/draw-trees)
(s/draw-hero 200 300)
(s/draw-enemy 500 320)
(ui/draw-hud)
(ui/draw-health-bar 20 20 200 20 0.7)

(render "game_scene.png")
```

**sprites.pml**
```pml
(provide draw-hero draw-enemy)

(define-texture hero-tex (32 64)
  (group (rect 0 0 32 64 :fill "#e94560") (circle 16 16 8 :fill "#ffffff")))

(defn draw-hero (x y)
  (add (texture-map (rect x y 32 64) :source hero-tex :mode :planar)))

(defn draw-enemy (x y)
  (add (circle (+ x 16) (+ y 16) 16 :fill "#ff0000")))
```


# 附录

## pml.exe 路径

```
G:/Project/PML/build/debug/src/pml/cli/Debug/pml.exe
```

## 运行命令

```bash
G:/Project/PML/build/debug/src/pml/cli/Debug/pml.exe 你的文件.pml
```
CWD 设为 `.pml` 文件所在目录。输出 PNG/GIF 在 `.pml` 同目录。

## 文件扩展名

- `.pml` — PML 源文件
- `.png` — 静态渲染输出
- `.gif` — 动画渲染输出

## 渲染流程

```
源文件 → Lexer → Parser → Expander → Evaluator → GraphicObject → SkiaBackend → PNG/GIF
```
