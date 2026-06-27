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

**CWD 设为 `.pml` 文件所在目录。渲染的图片输出在 `.pml` 文件同目录下。**（模块化项目中 `import` 路径相对于入口文件目录。）

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
```pml
(provide rounded-box)

(defn rounded-box (x y w h)
  "Draw a rounded rectangle with configurable corner radius"
  (rect x y w h :rx 8
        :fill "#74b9ff"
        :stroke "#0984e3"
        :stroke-width 2))
```

---

## 1. 图形基元

形状函数全部是内置函数（不是特殊形式），接受位置参数 + 关键字参数。

所有形状共用的关键字参数：

| 参数 | 说明 |
|------|------|
| `:fill color` | 填充色（字符串或 color/rgb 返回值） |
| `:stroke color` | 描边色 |
| `:stroke-width n` | 线宽 |
| `:opacity n` | 不透明度（0.0–1.0，默认 1.0），别名 `:op` |
| `:fill-style` | 填充样式（用于粗糙模式） |
| `:blend-mode mode` | 混合模式字符串 |
| `:roughness n` | 手绘粗糙度（0-2，默认 1） |
| `:bowing n` | 手绘弯曲度（0-2，默认 0.5） |
| `:seed n` | 手绘随机种子 |
| `:stroke-align` | 描边对齐（`"center"`/`"inside"`/`"outside"`） |
| `:style` | 引用预定义的样式名称（符号） |

### `circle`
**签名**: `(circle cx cy r [:fill] [:stroke] [:stroke-width] [:roughness] [:bowing] [:seed] [:blend-mode] [:style])`

```pml
(add (circle 100 100 50 :fill "#ff0000"))
(add (circle 200 100 40 :fill "#3498db" :roughness 1.5 :bowing 0.8 :seed 42))
```

### `rect`
**签名**: `(rect x y w h [:rx radius] [:fill] [:stroke] [:stroke-width] [:roughness] [:bowing] [:seed] [:blend-mode] [:style])`

```pml
(add (rect 50 50 100 100 :fill "#00ff00" :rx 8))
```

### `ellipse`
**签名**: `(ellipse cx cy rx ry [:fill] [:stroke] [:stroke-width] [:roughness] [:bowing] [:seed] [:blend-mode] [:style])`

```pml
(add (ellipse 100 100 80 40 :fill "#0000ff"))
```

### `line`
**签名**: `(line x1 y1 x2 y2 [:stroke] [:stroke-width] [:roughness] [:bowing] [:seed] [:blend-mode] [:style])`

```pml
(add (line 10 10 190 190 :stroke "#ff00ff" :stroke-width 3))
```

### `polygon`
**签名**: `(polygon points [:fill] [:stroke] [:stroke-width] [:edge-noise] [:edge-mask] [:edge-seed] [:edge-subdiv] [:corner-radius] [:corner-mask] [:roughness] [:bowing] [:seed] [:blend-mode] [:style])`

`points` 为坐标对列表：`((x1 y1) (x2 y2) ...)` 或平铺的坐标列表 `(x1 y1 x2 y2 ...)`

额外的边缘扰动参数：
- `:edge-noise` — 边缘噪声强度（float 或 list）
- `:edge-mask` — 哪些边受噪声影响（bool 或 list）
- `:edge-seed` — 噪声随机种子
- `:edge-subdiv` — 细分次数
- `:corner-radius` — 圆角半径（float 或 list）
- `:corner-mask` — 哪些角圆角化（bool 或 list）

```pml
(add (polygon (list (list 100 20) (list 180 140) (list 20 140))
      :fill "#ff8800" :stroke "#000"))
```

### `path`
**签名**: `(path [:d svg-path-string] [:fill] [:stroke] [:stroke-width] [:roughness] [:bowing] [:seed] [:blend-mode] [:style])`

支持两种模式：通过 `:d` 传递 SVG 路径字符串，或传递命令列表。

```pml
(add (path :d "M10 10 C 20 20, 40 20, 50 10" :stroke "#333" :stroke-width 2))
```

### `text`
**签名**: `(text x y content [:fill color] [:font-size size] [:align 'left|'center|'right] [:blend-mode mode])`

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

## 2. 样式与颜色

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
| `(color/rgb r g b)` | 0-255 整数分量创建颜色 |
| `(color/rgba r g b a)` | a 为 0-255 整数或 0.0-1.0 浮点数 |

### 混合模式

在形状上通过 `:blend-mode "mode-name"` 设置：

`normal` `multiply` `screen` `overlay` `darken` `lighten` `color-dodge`
`color-burn` `hard-light` `soft-light` `difference` `exclusion` `hue`
`saturation` `color` `luminosity`

```pml
(add (rect 50 50 100 100 :fill "#ff0000" :blend-mode "multiply"))
```

### 样式函数

| 函数 | 说明 |
|------|------|
| `(fill go color)` | 设置填充色 |
| `(stroke go color)` | 设置描边色 |
| `(no-fill go)` | 移除填充 |
| `(no-stroke go)` | 移除描边 |
| `(stroke-width go w)` | 设置描边宽度 |

这些函数返回修改后的 GraphicObject（不可变风格）。

```pml
(add (fill (circle 100 100 50) "#ff0000"))
```

### 综合示例：样式

```pml
(set-backend! "skia")
(canvas 300 300 :bg "#ffffff")

(add (circle 100 100 80 :fill (color/rgba 255 0 0 100)))
(add (circle 150 100 80 :fill (color/rgba 0 0 255 100)))
(add (circle 125 150 80 :fill (color/rgba 0 255 0 100)))

(add (text 150 270 "Blend Demo" :fill "#333" :font-size 14 :align 'center))
(render "blend.png")
```

---

## 3. 变换

### 变换矩阵

| 函数 | 说明 |
|------|------|
| `(translate x y)` | 创建平移矩阵 |
| `(rotate deg)` | 创建旋转矩阵（角度制） |
| `(scale x y)` | 创建缩放矩阵 |
| `(shear shx shy)` | 创建斜切矩阵 |
| `(identity-matrix)` | 返回单位矩阵 |
| `(compose t1 t2)` | 组合两个变换矩阵（先 t2 后 t1） |
| `(matrix-inverse t)` | 变换矩阵的逆（不可逆时返回 nil） |
| `(matrix-apply t x y)` | 对点应用变换，返回 `(x' y')` 列表 |
| `(matrix? v)` | 判断是否为变换矩阵 |

所有函数返回 `AffineTransform` 值（`matrix-apply` 和 `matrix?` 除外）。

### `with-transform`
**签名**: `(with-transform graphic-object :transform matrix)`

```pml
(add (with-transform (rect -50 -50 100 100 :fill "#e17055")
       :transform (compose (translate 200 150) (rotate 45))))
```

### `translate-object` / `rotate-object` / `scale-object`

| 函数 | 说明 |
|------|------|
| `(translate-object go tx ty)` | 平移 GraphicObject |
| `(rotate-object go angle-deg)` | 旋转 GraphicObject |
| `(scale-object go sx [sy])` | 缩放 GraphicObject |

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

## 4. Canvas 与渲染

### `set-backend!`
**签名**: `(set-backend! "skia")`

设置渲染后端。必须在创建 `canvas` 之前调用。

| 函数 | 说明 |
|------|------|
| `(set-backend! name)` | 设置渲染后端（字符串或符号） |
| `(current-backend)` | 返回当前后端名称 |
| `(list-backends)` | 列出所有可用后端 |
| `(backend-capabilities)` | 返回当前后端能力列表 |
| `(backend-available? name)` | 判断后端是否可用 |

### `canvas`
**签名**: `(canvas w h [:bg color])`

创建画布。`:bg` 设置背景色（默认 `"#FFFFFF"`）。返回 Canvas 对象，可用于多画布操作。

### `sprite-canvas`
**签名**: `(sprite-canvas w h [:bg color] [:anchor anchor] [:padding n])`

创建精灵画布（用于导出到 spritesheet）。

- `:bg` — 背景色（默认 `"transparent"`）
- `:anchor` — 锚点位置（默认 `"center-bottom"`，可选 `"top-left"` 等）
- `:padding` — 内边距（默认 0）

### `clear-canvas`
**签名**: `(clear-canvas)`

清空当前画布中的所有图形对象。用于动画循环中复用画布。

### `add`
**签名**: `(add graphic-object)` 或 `(add canvas graphic-object)`

将图形对象添加到画布。

- **单参数**：`(add shape)` — 添加到当前画布
- **双参数**：`(add canvas shape)` — 添加到指定画布对象
- **z-order**：后添加的图形在上层，会覆盖下层内容
- **返回**：`nil`

```pml
(let ((my-canvas (canvas 200 200)))
  (add (circle 50 50 30 :fill "#ff0000"))       ;; 添加到当前画布
  (add my-canvas (rect 10 10 80 80 :fill "#00f")) ;; 添加到指定画布
  (render "output.png"))
```

### `render`
**签名**: `(render filename [:format fmt] [:fps n] [:duration d])`

渲染当前画布为图片。输出格式由文件扩展名决定（`.png` / `.gif`）。

- `:format` — 强制输出格式（如 `"PNG"`、`"GIF"`）
- `:fps` — 帧率，> 0 时启用动画渲染（默认 0）
- `:duration` — 覆盖动画总时长

```pml
(set-backend! "skia")
(canvas 400 400 :bg "#16213e")
(add (circle 200 200 100 :fill "#e94560"))
(render "output.png")
```

### `render-set`
**签名**: `(render-set name :content graphic-object [:scales (1 2 4)] [:base-size (64 64)] [:format "PNG"])`

渲染多分辨率图片集。`:content` 指定图形对象，`:scales` 为倍率列表，`:base-size` 为基准尺寸。

```pml
(render-set "icon" :content (circle 16 16 16 :fill "#e94560")
  :scales '(1 2 4) :base-size '(32 32))
```

### `render-spritesheet`
**签名**: `(render-spritesheet filename sprite... [:cols n] [:cell-width w] [:cell-height h] [:padding n] [:bg color])`

- `:cols` — 列数（默认 4）
- `:cell-width` — 单元格宽度（默认 64）
- `:cell-height` — 单元格高度（默认 64）
- `:padding` — 间距（默认 2）
- `:bg` — 背景色（默认 `"transparent"`）

```pml
(render-spritesheet "tiles.png"
  (rect 0 0 32 32 :fill "#ff0000")
  (rect 0 0 32 32 :fill "#00ff00")
  (rect 0 0 32 32 :fill "#0000ff")
  :cols 3 :cell-width 32 :cell-height 32)
```

### `render-channels`
**签名**: `(render-channels graphic-object :output prefix [:width w] [:height h] [:channels symbols] [:bg color])`

渲染多通道图像（albedo、specular、normal 等）。

- `:output` — 输出文件前缀（必需）
- `:channels` — 通道列表（默认 `'(albedo specular normal)`）
- `:width` / `:height` — 渲染尺寸
- `:bg` / `:bg` — 背景色

### 综合示例：合成

```pml
(set-backend! "skia")
(canvas 300 300 :bg "#2d3436")

(add (rect 0 0 300 300 :fill "#0984e3"))
(add (circle 250 50 40 :fill "#fdcb6e"))
(add (rect 0 200 300 100 :fill "#00b894"))
(add (rect 80 120 100 120 :fill "#dfe6e9"))
(add (polygon (list (list 60 120) (list 130 70) (list 200 120)) :fill "#e17055"))
(add (rect 120 180 20 60 :fill "#636e72"))
(add (text 150 285 "My House" :fill "#fff" :font-size 12 :align 'center))
(render "house.png")
```

---

## 5. 纹理与 UV 映射

### `define-texture`（特殊形式）
**签名**: `(define-texture name (width height) body...)`

定义一个可复用的纹理。第一个参数（名称）不被求值，第二个是尺寸列表，后续表达式构成纹理内容。

```pml
(define-texture checker (64 64)
  (group
    (rect 0  0 32 32 :fill "#e94560")
    (rect 32 0 32 32 :fill "#16213e")
    (rect 0 32 32 32 :fill "#16213e")
    (rect 32 32 32 32 :fill "#e94560")))
```

### `texture-map`
**签名**: `(texture-map shape :source texture [:mode mode] [:wrap-x mode] [:wrap-y mode] [:filter mode] [:uv-vertices ((x y) ...)] [:edge-blend n] [:perspective-correction bool])`

参数：
- `:source` — 纹理名称（必需）
- `:mode` — UV 模式：`:planar`（平面）、`:harmonic`（调和，默认）、`:explicit`（显式）
- `:wrap-x` / `:wrap-y` — 包裹模式：`:clamp`（默认）、`:repeat`、`:mirror`
- `:filter` — 过滤模式：`:linear`（默认）、`:nearest`
- `:uv-vertices` — 显式 UV 坐标列表（`:explicit` 模式）
- `:edge-blend` — 边缘混合像素数（默认 0）
- `:perspective-correction :true` — 启用透视校正（默认关闭）。当多边形为梯形时，校正 UV 插值以避免透视失真

```pml
(add (texture-map (rect 40 40 320 160 :rx 8)
      :source checker :wrap-x :repeat :mode :planar))

;; 透视校正：将棋盘格纹理映射到梯形，显示正确的透视
(define-texture grid (64 64)
  (group
    (rect 0  0 32 32 :fill "#FF0000")
    (rect 32 0 32 32 :fill "#00FF00")
    (rect 0 32 32 32 :fill "#0000FF")
    (rect 32 32 32 32 :fill "#FFFF00")))
(add (texture-map (polygon '((10 10) (190 10) (170 190) (30 190)))
      :source grid :mode :explicit
      :uv-vertices '((0 0) (1 0) (1 1) (0 1))
      :perspective-correction :true))
```

### `render-to-texture`
**签名**: `(render-to-texture width height graphic-object)`

将 GraphicObject 渲染为纹理值（TextureBox）。可用于纹理合成：

```pml
(define tex (render-to-texture 64 64
  (group
    (rect 0 0 32 32 :fill "#e94560")
    (rect 32 0 32 32 :fill "#16213e")
    (rect 0 32 32 32 :fill "#16213e")
    (rect 32 32 32 32 :fill "#e94560"))))
(add (texture-map (rect 0 0 200 200) :source tex :mode :planar))
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

## 6. 着色器与程序化纹理

PML 的着色器系统允许编译 SkSL（Skia Shader Language）片段，或通过内置噪声函数生成程序化纹理，然后通过 `apply-shader!` 附加到图形对象上。

着色器句柄是一个不透明的整数 ID，由后端管理。噪声函数和 SkSL 编译都返回句柄。

### 着色器基本操作

#### `shader`
**签名**: `(shader source-string)`

编译一段 SkSL 着色器源代码，返回着色器句柄。

```pml
(define my-shader (shader "
  half4 main(float2 xy) {
    return half4(xy.x / 200.0, xy.y / 200.0, 0.5, 1.0);
  }
"))
```

#### `apply-shader!`
**签名**: `(apply-shader! graphic-object shader-handle [:coordinate-space :local | :world])`

返回一个绑定了着色器的新 GraphicObject。着色器会覆盖该对象的填充。

- `:coordinate-space` — 坐标空间（默认 `:local` 对象局部坐标；`:world` 画布世界坐标）。世界坐标用于需要对齐场景的着色效果（如背景纹理）

```pml
;; 将着色器附加到矩形，使用局部坐标
(add (apply-shader! (rect 50 50 200 200) my-shader))

;; 世界坐标着色器 — 着色效果不随对象位置偏移
(add (apply-shader! (rect 50 50 200 200) my-shader
      :coordinate-space :world))
```

### 噪声着色器

所有噪声函数返回着色器句柄，可直接传入 `apply-shader!`。

#### `noise-fractal`
**签名**: `(noise-fractal [:freq-x 0.02] [:freq-y 0.02] [:octaves 4] [:seed 0] [:tile-w 0] [:tile-h 0] [:lacunarity 2.0] [:persistence 0.5])`

Perlin 分形噪声，平滑连续的噪声图案。适合地形、云朵、纹理。

- `:freq-x` / `:freq-y` — 噪声频率（越小图案越大）
- `:octaves` — 叠加的噪声层数（1–32，越高细节越丰富）
- `:seed` — 随机种子
- `:tile-w` / `:tile-h` — 无缝平铺尺寸（>0 启用无缝平铺）
- `:lacunarity` — 每层频率倍增系数（1.0–10.0，默认 2.0）
- `:persistence` — 每层振幅衰减系数（0.0–1.0，默认 0.5）

```pml
(define n (noise-fractal :seed 42 :octaves 6 :freq-x 0.01 :freq-y 0.01))
(add (apply-shader! (rect 0 0 400 400) n))
```

#### `noise-turbulence`
**签名**: `(noise-turbulence [:freq-x 0.02] [:freq-y 0.02] [:octaves 4] [:seed 0] [:tile-w 0] [:tile-h 0] [:lacunarity 2.0] [:persistence 0.5])`

Perlin 湍流噪声，比分形噪声更尖锐、更"破碎"。适合火焰、烟雾、大理石纹理。

参数与 `noise-fractal` 相同。

```pml
(define n (noise-turbulence :seed 99 :octaves 4 :freq-x 0.02))
(add (apply-shader! (rect 0 0 400 400) n))
```

#### `noise-voronoi`
**签名**: `(noise-voronoi [:cell-size 32] [:seed 0] [:jitter 0.5])`

Voronoi / 细胞噪声，生成细胞状图案。适合石头、鳞片、蜂巢纹理。

- `:cell-size` — 细胞大小（像素，1–1024，默认 32）
- `:seed` — 随机种子
- `:jitter` — 特征点随机偏移量（0.0–1.0，默认 0.5；越大细胞形状越不规则）

```pml
(define v (noise-voronoi :cell-size 40 :seed 7 :jitter 0.3))
(add (apply-shader! (rect 0 0 400 400) v))
```

### 噪声组合

#### `noise-warp`
**签名**: `(noise-warp base-handle warp-handle [:amount 30.0] [:freq 0.01])`

域扭曲（Domain Warp）：用一个噪声场扭曲另一个噪声的采样坐标，产生流动、破碎的有机纹理。

- `base-handle` — 基础噪声句柄
- `warp-handle` — 扭曲场噪声句柄
- `:amount` — 扭曲强度（像素，默认 30.0）
- `:freq` — 扭曲场采样频率（默认 0.01）

```pml
(define base (noise-fractal :seed 1 :octaves 4))
(define warp (noise-turbulence :seed 2 :octaves 3))
(define warped (noise-warp base warp :amount 40.0 :freq 0.008))
(add (apply-shader! (rect 0 0 400 400) warped))
```

#### `noise-blend`
**签名**: `(noise-blend noise-a noise-b [:mode 'gradient | 'horizontal | 'vertical] [:weight 0.5])`

在两个噪声着色器之间渐变过渡。

- `:mode` — 过渡模式
- `:weight` — 过渡中点位置（0.0–1.0，默认 0.5）

```pml
(define a (noise-fractal :seed 1 :octaves 4))
(define b (noise-turbulence :seed 2 :octaves 4))
(define blended (noise-blend a b :mode 'horizontal :weight 0.5))
(add (apply-shader! (rect 0 0 400 400) blended))
```

#### `quantize-noise`
**签名**: `(quantize-noise noise-handle :levels '((threshold color) ...))`

将连续噪声量化为离散色带。适合生成地形图、等高线风格。

- `noise-handle` — 噪声着色器句柄
- `:levels` — `(threshold color)` 对列表，按 threshold 升序排列。最后一项 threshold 应为 1.0

```pml
(define n (noise-fractal :seed 42 :octaves 6))
(define q (quantize-noise n
  :levels '((0.33 "#2d5a2d") (0.66 "#6abf4a") (1.0 "#4a9bd6"))))
(add (apply-shader! (rect 0 0 400 400) q))
```

### Uniform 数据

着色器通过 `uniform` 变量接收外部参数。

#### `make-uniforms`
**签名**: `(make-uniforms float1 float2 ...)`

将一组浮点数打包为原始 uniform 数据字节串。

```pml
(define uniforms (make-uniforms 0.5 1.0 0.25))
```

#### `apply-uniforms`
**签名**: `(apply-uniforms shader-handle uniform-data-string)`

创建绑定了 uniform 数据的新着色器句柄。

```pml
(define s-with-u (apply-uniforms my-shader (make-uniforms 0.5)))
(add (apply-shader! (rect 0 0 200 200) s-with-u))
```

#### `uniform-float`
**签名**: `(uniform-float shader-handle name value)`

便捷函数：设置单个 float uniform，返回新句柄。

```pml
(define s (uniform-float my-shader "u_time" 1.5))
```

### 着色器应用示例

```pml
(set-backend! "skia")
(canvas 400 400 :bg "#1a1a2e")

;; 分形噪声 — 云朵背景（世界坐标）
(define cloud (noise-fractal :seed 42 :octaves 6 :freq-x 0.008 :freq-y 0.008))
(add (apply-shader! (rect 0 0 400 400) cloud :coordinate-space :world))

;; Voronoi — 石头纹理圆
(define stone (noise-voronoi :cell-size 30 :seed 13 :jitter 0.4))
(add (apply-shader! (circle 200 200 80) stone))

;; 域扭曲 — 流动火焰效果
(define fire-base (noise-turbulence :seed 7 :octaves 5 :freq-x 0.015 :freq-y 0.015))
(define fire-warp (noise-turbulence :seed 8 :octaves 3))
(define fire (noise-warp fire-base fire-warp :amount 50.0 :freq 0.005))
(add (apply-shader! (rect 100 300 200 80 :rx 12) fire))

(render "shader_demo.png")
```

---

## 7. Rough 风格（手绘效果）

通过给普通形状添加 `:roughness`、`:bowing`、`:seed` 参数启用手绘效果：

```pml
;; 在 circle/rect 等形状上加 roughness 参数
(add (circle 100 100 60 :fill "#e17055" :roughness 1.5 :bowing 0.8 :seed 42))
(add (rect 50 50 100 100 :fill "#74b9ff" :roughness 0.5 :seed 1))
```

参数范围：`:roughness` 0-2（默认 1），`:bowing` 0-2（默认 0.5），`:seed` 任意整数。

### 综合示例

```pml
(set-backend! "skia")
(canvas 400 300 :bg "#fff9ef")

(add (rect 30 30 140 100 :fill "#74b9ff" :roughness 0.5 :seed 1))
(add (circle 270 80 50 :fill "#ff7675" :roughness 1.5 :bowing 0.3 :seed 2))
(add (line 60 200 340 220 :stroke "#636e72" :stroke-width 2 :roughness 1 :seed 3))
(add (polygon (list (list 200 250) (list 280 280) (list 120 280))
      :fill "#fdcb6e" :roughness 2 :seed 4))

(render "rough_demo.png")
```

---

## 8. 边缘扰动

### `perturb-polygon`
**签名**: `(perturb-polygon points [:edge-noise n] [:edge-subdiv n] [:corner-radius n] [:seed n] [:fill color] [:stroke color] [:stroke-width w])`

对多边形边缘添加随机扰动，生成有机形状。

```pml
(add (perturb-polygon '((0 0) (200 0) (200 200) (0 200))
      :edge-noise 15.0 :edge-subdiv 4 :seed 42 :fill "#e94560"))
```

参数说明：
- `:edge-noise` — 噪声强度（float 或 float 列表，每边独立）
- `:edge-mask` — 哪些边受噪声影响（bool 或列表）
- `:edge-subdiv` — 每条边的细分次数
- `:corner-radius` — 圆角半径
- `:corner-mask` — 哪些角圆角化
- `:fill` / `:stroke` / `:stroke-width` — 样式传透

---

## 9. 动画

### `animate`
**签名**: `(animate target property from to duration [:ease name] [:delay n] [:repeat n])`

创建一个动画。`target` 为 GraphicObject，`property` 为属性名字符串（如 `"x"`、`"y"`、`"transform.tx"`、`"fill"`），`from`/`to` 为起止值，`duration` 为时长（秒）。

- `:ease` — 缓动函数名（默认 `"linear"`）
- `:delay` — 延迟启动（秒）
- `:repeat` — 重复次数（整数或 `'infinite`）

```pml
(animate (circle 100 100 30 :fill "#e94560") "y" 50 250 1.0 :ease "ease-in-out")
(animate ball "x" 0 200 1.5 :repeat 'infinite)
```

### `play` / `stop` / `pause` / `seek`

| 函数 | 说明 |
|------|------|
| `(play)` | 开始播放 |
| `(stop)` | 停止并重置 |
| `(pause)` | 暂停播放 |
| `(seek t)` | 跳转到时间 t |
| `(seek anim t)` | 跳转到指定动画的指定时间 |
| `(animation-state)` | 获取当前动画状态（`"playing"`/`"paused"`/`"stopped"`） |

### `every-frame`
**签名**: `(every-frame thunk)` — 每帧调用回调函数（无参过程）。

```pml
(every-frame (lambda () (println "frame!")))
```

### `parallel` / `sequence`
**签名**: `(parallel anims...)` — 所有动画同时播放。
**签名**: `(sequence anims...)` — 动画依次播放。

```pml
(let ((ball (circle 150 50 30 :fill "#e94560")))
  (add ball)
  (sequence
    (animate ball "y" 50 250 1.0 :ease "ease-in-out")
    (animate ball "y" 250 50 1.0 :ease "ease-in-out")))
```

### 综合示例

```pml
(set-backend! "skia")
(canvas 300 300 :bg "#1a1a2e")

(let ((ball (circle 150 50 30 :fill "#e94560")))
  (add ball)
  (animate ball "y" 50 250 1.0 :ease "ease-in-out")
  (animate ball "x" 150 250 0.6))

(render "animation.gif")
```

---

## 10. 图层合成

图层系统基于 `make-layer` / `make-composition` / `composition-add` / `layer-with` 等函数。

### `make-layer`
**签名**: `(make-layer name graphic-object [:transform matrix] [:offset (x y)] [:anchor anchor] [:opacity n] [:blend mode] [:visible bool] [:locked bool] [:filter filter-value])`

创建一个命名的图层。

- `:anchor` — 锚点（默认 `"top-left"`）
- `:blend` — 混合模式（默认 `"normal"`）
- `:filter` — 单个滤镜或滤镜列表

```pml
(define bg-layer (make-layer "bg" (rect 0 0 200 200 :fill "#0984e3")))
(define fg-layer (make-layer "fg" (circle 100 100 50 :fill "#fdcb6e")))
```

### `make-group`
**签名**: `(make-group name layers...)` — 将多个图层编组。

### `make-composition`
**签名**: `(make-composition name width height [:fps n] [:bg color])` — 创建合成。

```pml
(define comp (make-composition "scene" 500 350 :fps 12 :bg "#ffffff"))
```

### `composition-add`
**签名**: `(composition-add composition layers...)` — 向合成中添加图层。

```pml
(composition-add comp bg-layer fg-layer)
```

### `layer-with`
**签名**: `(layer-with layer [:transform matrix] [:offset (x y)] [:anchor anchor] [:opacity n] [:blend mode] [:visible bool] [:locked bool] [:filter filter-value])`

创建图层的修改副本（不可变风格）。

### `composition-render` / `composition-animate`

| 函数 | 说明 |
|------|------|
| `(composition-render comp filename [:output-dir "."])` | 渲染合成为 PNG |
| `(composition-animate comp basename [:frames n] [:fps n] [:cols n] [:padding n] [:output-dir "."] [:meta "aseprite"] [:step-fn proc])` | 导出动画帧 |
| `(project-render-all [:output-dir "."])` | 渲染所有项目合成 |

### 类型判断
- `(layer? v)` — 判断是否为图层
- `(composition? v)` — 判断是否为合成

### 滤镜应用示例

滤镜通过 `make-layer` 的 `:filter` 参数应用到图层：

```pml
(define layer-with-shadow
  (make-layer "shadow-box"
    (rect 0 0 120 120 :fill "#3498DB" :rx 8)
    :offset '(70 80)
    :filter (drop-shadow :dx 8 :dy 8 :blur 8 :color "#00000060")))
```

---

## 11. 图像滤镜

所有滤镜函数都接受**纯关键字参数**（没有位置参数），返回 `ImageFilter` 值。
滤镜通过 `make-layer` 的 `:filter` 参数应用，或通过 `filter-chain` 组合。

### 颜色调整

| 函数 | 关键字参数 | 说明 |
|------|------------|------|
| `(color-adjust :brightness 0.0 :contrast 1.0 :saturation 1.0 :hue 0.0 :exposure 0.0 :gamma 1.0 :temperature 6500 :tint 0.0 :grayscale 0.0 :sepia 0.0 :invert #f)` | 全部可选 | 综合颜色调整 |
| `(levels :in-low 0.0 :gamma 1.0 :in-high 1.0 :out-low 0.0 :out-high 1.0)` | 全部可选 | 色阶调整 |
| `(curves :channel "rgb" :points ((0 0) (255 255)))` | 全部可选 | 曲线调整 |
| `(threshold :value 128)` | 可选 | 二值化 |
| `(posterize :levels 4)` | 可选 | 色调分离 |

### 模糊与锐化

| 函数 | 关键字参数 | 说明 |
|------|------------|------|
| `(blur :radius 1.0 [:type "gaussian"|"box"|"motion"] [:angle 0])` | 可选 | 模糊 |
| `(sharpen :amount 1.0 :radius 1.0)` | 可选 | 锐化 |
| `(edge-detect :type "sobel"|"laplacian")` | 可选 | 边缘检测 |
| `(emboss :direction 45.0)` | 可选 | 浮雕 |

### 卷积

```pml
(convolution :width 3 :height 3
  :kernel (0 -1 0 -1 5 -1 0 -1 0)
  :offset 0 :divisor 1.0)
```

### 图层样式

| 函数 | 关键字参数 | 说明 |
|------|------------|------|
| `(drop-shadow :dx n :dy n :blur n :color color)` | 全部必需 | 投影 |
| `(inner-shadow :dx n :dy n :blur n :color color)` | 全部必需 | 内阴影 |
| `(outer-glow :blur n :color color)` | 全部必需 | 外发光 |
| `(inner-glow :blur n :color color)` | 全部必需 | 内发光 |
| `(bevel-emboss :angle n :altitude n :blur n :highlight color :shadow color)` | 全部可选 | 斜面浮雕 |

### 滤镜组合

```pml
(filter-chain (blur :radius 5) (drop-shadow :dx 2 :dy 2 :blur 4 :color "#000"))
```

### 类型判断
- `(filter? v)` — 判断是否为滤镜值

### 滤镜应用示例

```pml
(make-layer "glow-box"
  (circle 100 100 80 :fill "#e94560")
  :filter (outer-glow :blur 12 :color "#ff6b6b"))

(make-layer "multi-filter"
  (rect 50 50 100 100 :fill "#3498DB")
  :filter (filter-chain
    (drop-shadow :dx 4 :dy 4 :blur 6 :color "#00000040")
    (blur :radius 1)))
```

---

## 12. 风格与调色板（Sprite 系统）

### `define-style`
**签名**: `(define-style name [:outline-width n] [:outline-color color] [:outline-style "solid"] [:shading "flat"] [:shadow bool] [:highlight bool] [:pixel-size n])`

定义一个视觉风格（用于精灵角色系统）。所有关键字参数可选。

```pml
(define-style hero
  :outline-width 2.5
  :outline-color "#333333"
  :shading "cel"
  :shadow #t
  :highlight #t)

(define-style pixel-char
  :outline-style "none"
  :shading "flat"
  :pixel-size 3)
```

### `use-style`
**签名**: `(use-style name)` — 按名称查找已定义的风格，返回 StyleDescriptor 值。

### `define-palette`
**签名**: `(define-palette "name" '(("key" "#color") ...))`

定义一个命名的颜色调色板。

```pml
(define-palette "ocean"
  '(("deep" . "#0f0c29")
    ("mid" . "#302b63")
    ("light" . "#24243e")))
```

### `palette-ref`
**签名**: `(palette-ref "key")` — 在当前活动调色板中查找颜色键。

---

## 13. 瓦片地图

### `define-tileset`
**签名**: `(define-tileset name :tile-size n :tiles '((id name [graphic-expr [detail-expr]]) ...))`

定义一个命名瓦片集。

- `:tile-size` — 瓦片尺寸（像素，默认 32）
- `:tiles` — 瓦片定义列表，每个条目：`(id name [graphic-expr [detail-expr]])`
  - `id` — 瓦片整数 ID
  - `name` — 瓦片名称（字符串或符号）
  - `graphic-expr` — 瓦片图形表达式（可选）
  - `detail-expr` — 细节覆盖层表达式（可选）

```pml
(define-tileset terrain
  :tile-size 32
  :tiles '((0 "grass" (rect 0 0 32 32 :fill "#4CAF50"))
           (1 "water" (rect 0 0 32 32 :fill "#2196F3"))
           (2 "sand"  (rect 0 0 32 32 :fill "#FFE082"))))
```

### `make-tilemap`
**签名**: `(make-tilemap tileset-name cols rows [:projection 'orthogonal|'isometric] [:layers n])`

创建一个瓦片地图实例（以 tileset 名称为名称）。

```pml
(make-tilemap terrain 10 8 :projection 'orthogonal :layers 1)
```

### `tilemap-set!`
**签名**: `(tilemap-set! tilemap-name layer col row tile-id)`

设置瓦片地图中指定位置的瓦片。

```pml
(tilemap-set! terrain 0 0 0 0)  ;; layer 0, col 0, row 0 → grass
(tilemap-set! terrain 0 1 0 1)  ;; layer 0, col 1, row 0 → water
```

### `render-tilemap`
**签名**: `(render-tilemap tilemap-name :output "out.png" [:bg "transparent"])`

渲染瓦片地图为 PNG 图片。

```pml
(render-tilemap terrain :output "map.png" :bg "#87CEEB")
```

---

## 14. 3D 图形

### 3D 形状

所有 3D 形状函数返回 `Mesh3D` GraphicObject。

| 函数 | 关键字参数 | 说明 |
|------|------------|------|
| `(cube3d :size 80 [:front go] [:back go] [:left go] [:right go] [:top go] [:bottom go])` | 全部可选 | 立方体 |
| `(cuboid3d :width 80 :height 80 :depth 80 [:front go] [:back go] [:left go] [:right go] [:top go] [:bottom go])` | 全部可选 | 长方体 |
| `(rounded-cuboid3d :width 80 :height 80 :depth 80 :radius 8 :segments 4 [:front go] [:back go] [:left go] [:right go] [:top go] [:bottom go])` | 全部可选 | 圆角长方体 |
| `(cone3d :radius 40 :height 80 :segments 32 [:side go] [:base go])` | 全部可选 | 圆锥体 |
| `(plane3d :width 80 :depth 80 [:material go])` | 全部可选 | 平面 |
| `(sphere3d :radius 40 :segments 32 :rings 16 [:material go])` | 全部可选 | 球体 |

每个面接受可选的 `GraphicObject` 作为材质（颜色/纹理）。

```pml
(add (cuboid3d :width 80 :height 80 :depth 80
      :front (circle 40 40 30 :fill "#e94560")))
```

### `camera`
**签名**: `(camera [:position (x y z)] [:target (x y z)] [:up (0 1 0)] [:fov n] [:size n] [:near n] [:far n] [:projection "perspective"|"orthographic"] [:backface-culling bool])`

设置 3D 相机参数。

```pml
(camera :position '(0 -50 200) :target '(0 0 0) :fov 45 :projection "perspective")
(add (cuboid3d :width 80 :height 80 :depth 80 :front (circle 40 40 30 :fill "#e94560")))
```

### 3D 变换

| 函数 | 说明 |
|------|------|
| `(rotate-x mesh angle)` | 绕 X 轴旋转 |
| `(rotate-y mesh angle)` | 绕 Y 轴旋转 |
| `(rotate-z mesh angle)` | 绕 Z 轴旋转 |
| `(translate3d mesh x y z)` | 3D 平移 |
| `(scale3d mesh sx sy sz)` | 3D 缩放 |

---

## 15. 模块系统

### `provide`
**签名**: `(provide symbol ...)` — 声明当前模块的导出符号。

```pml
(provide my-func my-var)
```

### `import`
**签名**: `(import "path" :prefix prefix)` — 加载模块并使用前缀导入。

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

## 16. 其他内建函数

### 多态访问器

| 函数 | 说明 |
|------|------|
| `(get container key)` | 从容器中获取值（列表→索引，向量→索引，哈希表→键） |
| `(set-at! container key value)` | 设置值（列表返回新列表，向量/哈希表就地修改） |

---

## 17. 综合项目示例

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
源文件 → Lexer → Parser → Expander → Evaluator → GraphicObject → RenderBackend → PNG/GIF
```
