---
name: pml
description: |
  PML (Picture Markup Language) — C++23 Lisp-style DSL for code-to-image generation.
  Teaches AI agents how to use PML builtins to create 2D/3D graphics, animations,
  tilemaps, shaders, game sprites, and multi-channel asset exports.

  Triggers:
  - "write pml code", "generate pml", "use pml", "pml builtins", "pml skill"
  - Any request involving PML language syntax, APIs, or rendering pipeline
  - When user asks to create images, animations, tilemaps, or game assets via PML
---

# PML — Picture Markup Language (C++23 Port)

PML 是一个 Lisp 风格的 DSL，用于代码生成 2D/3D 图像、动画和游戏素材。
C++23 实现，Skia 后端渲染，GIF 动画输出。

---

## 1. 快速识别

| 项目 | 说明 |
|------|------|
| 文件后缀 | `.pml` |
| 语法 | Lisp S-表达式：`(function arg1 arg2 :kwarg val)` |
| 编码 | UTF-8（CJK 文字渲染受限于 Skia 字体 fallback） |
| 渲染后端 | Skia（GPU）+ GIF（CPU） |
| 构建 | `cmake --preset debug && cmake --build --preset debug` |
| 运行 | `pml.exe file.pml` / `pml.exe`（REPL） |

### 核心概念

PML 程序的输出是**图像文件**。工作流总是：
```
图形对象 (GraphicObject) → 样式化 → 变换 → Canvas 组装 → 渲染到文件
```

- **GraphicObject**: 不可变的形状/图像/组。所有形状 builtin 返回 GraphicObject 值。
- **Canvas**: 收集 GraphicObjects 的累积器。`add` 放入，`render` 输出。
- **Value**: 一切皆值。数、字符串、列表、GraphicObject、模块——都是 `Value` 类型。

---

## 2. 代码模式

### 模式 1：直接渲染（单次输出）

```scheme
; 构建 GraphicObject，直接渲染到文件（无需 Canvas）
(render (circle 50 50 30 :fill "#e74c3c") :format "PNG")
```

### 模式 2：Canvas 管线（多次 `add` 后输出）

```scheme
(let ((c (canvas)))
  (add c (rect 10 10 80 80 :fill "#3498db"))
  (add c (circle 50 50 30 :fill "#e74c3c" :opacity 0.7))
  (render c "output.png"))
```

### 模式 3：命名 Sprite + 渲染

```scheme
(define-style 'my-style :fill "#2ecc71" :stroke "#27ae60")
(define my-sprite (group
  (rect 0 0 64 64 :fill "#34495e")
  (circle 32 32 20 :fill "#e67e22")))
(render my-sprite "sprite.png")
```

### 模式 4：Tilemap 管线

```scheme
(define-tileset 'terrain :tile-size 32
  :tiles '((1 grass  (rect 0 0 32 32 :fill "#2ecc71"))
           (2 stone  (rect 0 0 32 32 :fill "#95a5a6"))))

(define tm (make-tilemap 'terrain 8 6 :layers 2))
(tilemap-set! tm 0 3 2 1)   ; layer 0, col=3, row=2 → grass
(tilemap-set! tm 1 3 2 2)   ; layer 1, col=3, row=2 → stone
(render-tilemap tm :output "terrain.png")
```

### 模式 5：动画

```scheme
(define sprite (circle 50 50 20 :fill "#e74c3c"))
(animate sprite :duration 2.0 :loops true
  (sequence
    (every-frame (lambda (t) (translate-object sprite (* t 100) 0)) 1.0)
    (every-frame (lambda (t) (translate-object sprite 0 (* t 100)) 1.0))))
(render "anim.gif" :format "GIF" :fps 12)
```

### 模式 6：多通道素材导出

```scheme
(define sword (group
  (rect 20 0 8 60 :fill "#bdc3c7")
  (polygon 24 60 16 80 32 80 :fill "#95a5a6")))
(render-channels sword :output "sword"
  :channels '(albedo normal specular))
; → sword_albedo.png, sword_normal.png, sword_specular.png
```

### 模式 7：SkSL 着色器 + 多纹理

```scheme
(define tex-shader (shader "
uniform shader base;
uniform shader overlay;
half4 main(float2 xy) {
  half4 b = base.eval(xy);
  half4 o = overlay.eval(xy);
  return half4(mix(b.rgb, o.rgb, o.a), b.a + o.a*(1-b.a));
}"))
(bind-textures tex-shader :textures
  '((base   (image "bg.png" 0 0 256 256))
    (overlay (circle 32 32 16 :fill "rgba(255,0,0,0.5)"))))
(apply-shader! tex-shader)
```

### 模式 8：模块化组织

```scheme
; --- tiles/shapes.pml ---
(provide grass stone)

(define grass (rect 0 0 32 32 :fill "#2ecc71"))
(define stone (rect 0 0 32 32 :fill "#95a5a6"))

; --- main.pml ---
(import "tiles/shapes.pml" as tiles)
(define-tileset 'terrain :tile-size 32
  :tiles '((1 grass tiles/grass)
           (2 stone tiles/stone)))
```

### 模式 9：滤镜链

```scheme
(define glow (filter-chain
  (drop-shadow :offset '(2 2) :radius 4 :color "rgba(0,0,0,0.5)")
  (blur :radius 2)))
(define panel (group
  (rect 0 0 100 60 :fill "#ecf0f1")
  (rect 5 5 90 50 :fill "#bdc3c7")
  (apply-filter glow (circle 50 30 15 :fill "#e74c3c"))))
```

### 模式 10：组合 3D

```scheme
(camera :position '(3 4 5) :target '(0 0 0) :projection 'perspective)
(define scene (group
  (cube3d 1 :fill "#e74c3c")
  (sphere3d 0.5 :fill "#3498db")))
; 在 Canvas 中渲染 3D 场景
```

### 模式 11：噪声生成无缝 tile 纹理

```scheme
; 1. 创建无缝分形噪声（tile-width/tile-height 确保四方连续）
(define n (noise-fractal :seed 42 :octaves 6
                         :freq-x 0.04 :freq-y 0.04
                         :tile-width 64 :tile-height 64))

; 2. 量化为离散色阶（4 级石板纹理）
(define stone (quantize-noise n
  :levels '((0.25 "#2a2a2a") (0.50 "#555555") (0.75 "#8a8a8a") (1.0 "#bbbbbb"))))

; 3. 应用到矩形得到 tile GraphicObject
(define stone-tile (apply-shader! (rect 0 0 64 64 :fill "white") stone))

; 4. 放入 tileset 使用
(define-tileset 'dungeon :tile-size 64
  :tiles '((1 floor stone-tile)))
(render-tilemap (make-tilemap 'dungeon 3 3) :output "floor.png")
```

### 模式 12：Voronoi 细胞噪声

```scheme
; Voronoi 噪声：细胞/马赛克纹理
(define cells (noise-voronoi :cell-size 32 :seed 0 :jitter 0.5))
(apply-shader! (rect 0 0 256 256 :fill "white") cells)

; 小细胞 + 低 jitter → 规则蜂窝
(define honeycomb (noise-voronoi :cell-size 16 :seed 0 :jitter 0.2))

; 大细胞 + 高 jitter → 有机破碎感
(define organic (noise-voronoi :cell-size 64 :seed 7 :jitter 0.9))

; 搭配 quantize-noise 做色阶地形
(define terrain (quantize-noise (noise-voronoi :cell-size 40 :seed 777 :jitter 0.7)
  :levels '((0.25 "#2d5a27") (0.50 "#6b8e23")
            (0.75 "#a0895c") (1.0  "#8b7355"))))
```

### 模式 13：域扭曲（Domain Warp）

```scheme
; 用噪声场扭曲另一个噪声的采样坐标
(define base (noise-fractal :octaves 6 :scale 0.025 :seed 1))
(define warp-field (noise-fractal :octaves 2 :scale 0.008 :seed 99))

; 柔和扭曲（小 amount → 轻微流动）
(define mild (noise-warp base warp-field :amount 20.0 :freq 0.008))

; 强扭曲（大 amount → 剧烈扭曲）
(define strong (noise-warp base warp-field :amount 80.0 :freq 0.005))

; 湍流噪声做扭曲场 → 破碎玻璃效果
(define rough (noise-warp base
  (noise-turbulence :octaves 4 :scale 0.015 :seed 77)
  :amount 50.0 :freq 0.01))

; 应用着色器
(apply-shader! (rect 0 0 256 256 :fill "white") strong)
```

### 模式 14：噪声混合过渡（Noise Blend）

...

---

### 模式 15：手绘粗糙风格（Rough-Style / Hand-Drawn）

```scheme
; 所有形状可选 :roughness :bowing :seed :fill-style kwargs
; roughness=0（缺省）= 精确 Skia 路径，无任何影响

; 1. 基础粗糙描边
(circle 50 50 30 :fill "#e74c3c" :stroke "#c0392b"
                 :stroke-width 2 :roughness 2)

; 2. 填充纹理（精确描边 + 填充纹理）
(rect 10 10 80 80 :fill "#3498db" :roughness 0
      :fill-style "hachure")  ; solid / hachure / zigzag / cross-hatch / dots / dashed

; 3. 粗糙描边 + 填充纹理
(ellipse 50 50 40 30 :fill "#2ecc71" :stroke "#27ae60"
                      :roughness 2 :fill-style "cross-hatch")

; 4. 确定性输出（相同 seed = 完全一致的结果）
(begin
  (define a (circle 20 20 40 :fill "#e74c3c" :roughness 3 :seed 42))
  (define b (circle 20 20 40 :fill "#e74c3c" :roughness 3 :seed 42))
  ; a 和 b 像素一致

; 5. 通过 define-style 复用
(define-style 'hand-drawn :roughness 2 :bowing 1.5 :seed 0
              :fill-style "hachure")
(rect 10 10 80 80 :fill "#e74c3c" :style 'hand-drawn)

; 6. 粗糙多边形
(polygon '((50 10) (90 80) (10 80))
         :fill "#f39c12" :roughness 2 :fill-style "zigzag")

; 7. 粗糙动画 → GIF
(define base (circle 100 100 60 :fill "#e74c3c" :roughness 3
                     :fill-style "hachure"))
(animate base :duration 2.0
  (every-frame (lambda (t) (translate-object base (* t 80) 0)) 1.0))
(render "rough.gif" :format "GIF" :fps 12)
```
---

### 模式 16：对象级混合模式与描边对齐

直接在形状 / image builtin 上使用 `:blend-mode` 和 `:stroke-align` kwargs，无需 Layer 系统。

```scheme
; ── :blend-mode — 形状与下层的颜色混合 ──
; 支持 "multiply" "screen" "overlay" "darken" "lighten" "difference" 等 16 种模式

; multiply：红色底 + 绿色圆 → 重叠区变暗
(define bg (rect 0 0 100 100 :fill "#ff6666"))
(define fg (circle 50 50 40 :fill "#00cc44" :blend-mode "multiply"))

; screen：暗底 + 亮色圆 → 发光效果
(define bg2 (rect 0 0 100 100 :fill "#222222"))
(define fg2 (circle 50 50 40 :fill "#ff8800" :blend-mode "screen"))

; overlay：中性灰底 + 亮色圆 → 对比增强
(define bg3 (rect 0 0 100 100 :fill "#888888"))
(define fg3 (circle 50 50 40 :fill "#ffcc00" :blend-mode "overlay"))

; 图片也支持 :blend-mode
(image "examples/assets/coin.png" :blend-mode "screen")
```

```scheme
; ── :stroke-align — 描边相对于形状边界的位置 ──
; "center"（默认）| "inside" | "outside"
; 适用于 rect / circle / ellipse / polygon（闭合形状）
; 不适用于 line / text（无封闭区域或无描边概念）

; inside：描边完全在内部，不改变外部尺寸（适合容器内边框）
(rect 10 10 100 100 :fill "none" :stroke "#333"
      :stroke-width 8 :stroke-align "inside")

; outside：描边完全在外部，保留内部空间完整（适合装饰外框）
(rect 10 10 100 100 :fill "none" :stroke "#333"
      :stroke-width 8 :stroke-align "outside")

; 组合使用：发光圆环
(group
  (rect 0 0 120 120 :fill "#1a1a2e")
  (circle 60 60 45 :fill "#e94560" :opacity 1.0 :blend-mode "screen")
  (circle 60 60 40 :fill "none" :stroke "#ffffff"
          :stroke-width 3 :stroke-align "inside"))
```

```scheme
; ── 完整 API 参考 → docs/api/19-blend-stroke.md ──
```


```scheme
; 在两个噪声之间做渐变过渡
(define noise-a (noise-voronoi :cell-size 24 :seed 0 :jitter 0.5))
(define noise-b (noise-fractal :octaves 5 :scale 0.03 :seed 42))

; 径向渐变混合
(define grad (noise-blend noise-a noise-b :mode 'gradient :weight 0.5))

; 水平渐变（从左到右过渡）
(define horiz (noise-blend noise-a noise-b :mode 'horizontal :weight 0.4))

; 垂直渐变（从上到下过渡）
(define vert (noise-blend noise-a noise-b :mode 'vertical :weight 0.6))

; 链式混合（三路：A→B 水平，再和 C 垂直）
(define mix-ab (noise-blend noise-a noise-b :mode 'horizontal :weight 0.5))
(define mix-abc (noise-blend mix-ab
  (noise-turbulence :octaves 4 :scale 0.02 :seed 77)
  :mode 'vertical :weight 0.5))
```

---

## 3. 关键设计原则

| 原则 | 说明 |
|------|------|
| **值语义** | GraphicObject 不可变。修改用 `with-*` 返回新对象 |
| **kwargs 风格** | 关键字参数用 `:key val` 传递，所有图形形状都接受 `:fill` `:stroke` `:stroke-width` |
| **模块隔离** | 每个模块有自己的环境，`provide` 显式导出，`prefix/symbol` 访问 |
| **Canvas 累积** | 同一个 Canvas 可以 `add` 多次，`render` 时一次性输出 |
| **内置函数无异常** | 所有 builtin 返回 `Result<T>`，出错返回错误值而非 throw |
| **3D 是扩展** | 3D 图形作为特殊 GraphicObject 存在，渲染时 2D/3D 混用走不同的渲染路径 |

---

## 4. 常见错误

### 路径错误
```scheme
; ❌
(image "assets/sprite.png" 0 0 32 32)
; ✅ 用 asset-path? 检查路径
(if (file-exists? "assets/sprite.png")
    (image "assets/sprite.png" 0 0 32 32)
    (rect 0 0 32 32 :fill "red"))
```

### 忘记 `provide`
```scheme
; 在模块中定义了符号但没有 provide
(define grass (rect 0 0 32 32 :fill "green"))
; → import 方拿不到 grass！
(provide grass)  ; ✅ 必须显式导出
```

### 混淆 Canvas 和 GraphicObject
```scheme
; ❌ Canvas 本身不是 GraphicObject
(render (canvas) "out.png")        ; 可以 — Canvas 可渲染
; ❌ GraphicObject 不能 add 到另一个 GraphicObject
(group (circle 10 10 5) (rect 0 0 10 10))  ; ✅ group 可以
```

### Tilemap 索引越界
```scheme
(tilemap-set! my-tm 0 100 100 1)  ; ❌ 运行时错误
```

### 模块循环引用
```scheme
; a.pml → import "b.pml" → import "a.pml"
; → circular import error! 自动检测并报错
```

---

## 5. 特殊形式

PML 的特殊形式不由 `def()` 注册，而是硬编码在求值器中。它们有特殊的求值规则（参数不一定全部求值）。

| 形式 | 签名 | 说明 |
|------|------|------|
| `define` | `(define name value)` | 定义变量 |
| `lambda` | `(lambda (params ...) body ...)` | 创建闭包函数 |
| `if` | `(if cond then-expr else-expr)` | 条件分支 |
| `cond` | `(cond (test expr) ... (else expr))` | 多分支条件 |
| `and` | `(and expr ...)` | 短路与 |
| `or` | `(or expr ...)` | 短路或 |
| `begin` | `(begin expr ...)` | 顺序求值，返回最后一个 |
| `set!` | `(set! name value)` | 修改已有变量 |
| `let` | `(let ((var val) ...) body ...)` | 局部绑定（并行求值） |
| `let*` | `(let* ((var val) ...) body ...)` | 局部绑定（串行求值） |
| `do` | `(do ((var init step) ...) (test result ...) body ...)` | 迭代循环 |
| `quote` | `(quote expr)` / `'expr` | 阻止求值 |
| `quasiquote` | `` `expr `` | 准引用（配合 `,` unquote） |
| `import` | `(import "path.pml" [as prefix])` | 加载模块 |
| `provide` | `(provide sym ...)` | 导出符号 |
| `define-macro` | `(define-macro (name params ...) body)` | 定义宏 |
| `defmacro` | `(defmacro name (params ...) body)` | 卫生宏（变量自动重命名） |
| `macroexpand` | `(macroexpand expr)` | 展开宏（调试用） |
| `assert` | `(assert expr [message])` | 测试断言 |
| `gensym` | `(gensym)` | 生成唯一符号 |

---

## 6. 模块系统

```
(import "path.pml")           → 加载，自动推断前缀（文件名）
(import "path.pml" as pref)   → 加载，指定前缀
(import "lib/shapes.pml")     → 相对路径（相对于引用文件目录）
(provide symbol1 symbol2)     → 导出符号到模块环境
模块/符号                      → 访问模块导出的符号
```

**模块查找路径**：
1. 原路径（绝对路径直接使用）
2. 原路径 + `.pml`
3. 相对于引用文件目录
4. 相对于引用文件目录 + `.pml`

---

## 7. 命令行

| 命令 | 说明 |
|------|------|
| `pml.exe file.pml` | 执行文件 |
| `pml.exe file.pml --json -o out/` | JSON 结果输出到目录 |
| `pml.exe file.pml --watch` | 监听文件变化自动重渲染 |
| `pml.exe` | 启动 REPL |
| `pml-mcp.exe` | MCP JSON-RPC 服务（stdio） |

---

## 8. 边界情况

- **透明背景**: PNG 输出默认透明背景，指定 `:bg` kwarg 可设颜色
- **空 tilemap**: 所有 tile-id 为 0 → 全透明输出
- **空 Canvas**: `(render (canvas) "out.png")` → 输出透明或指定背景色图像
- **无导出**: 模块未 `provide` 任何符号 → `module-exports` 返回空列表
- **多 Layer Tilemap**: `render-tilemap` 默认渲染所有可见层，layer 0 在最底

---

## 9. 完整的文档参考

每个分类的详细 API 签名见 `docs/api/` 目录：
- [01-core-language.md](api/01-core-language.md) — 特殊形式 + 核心函数
- [02-arithmetic-logic.md](api/02-arithmetic-logic.md) — 算术 / 比较 / 谓词
- [03-collections.md](api/03-collections.md) — 列表 / 向量 / 哈希 / 字符串
- [04-modules-macros.md](api/04-modules-macros.md) — 模块 + 宏
- [05-graphics-shapes.md](api/05-graphics-shapes.md) — 2D 形状 + 样式 + 颜色
- [06-transforms-anchor.md](api/06-transforms-anchor.md) — 变换 + 矩阵
- [07-canvas-render.md](api/07-canvas-render.md) — Canvas 管线 + render/render-set
- [08-sprite-system.md](api/08-sprite-system.md) — Style / Palette / Components
- [09-animation.md](api/09-animation.md) — 动画时间线
- [10-tilemap.md](api/10-tilemap.md) — Tilemap 系统
- [11-render-channels.md](api/11-render-channels.md) — 多通道导出
- [12-shaders-textures.md](api/12-shaders-textures.md) — SkSL / Noise / Bind-Textures
- [13-assets-bitmaps.md](api/13-assets-bitmaps.md) — 图片加载 / Bitmap
- [14-3d-graphics.md](api/14-3d-graphics.md) — 3D 图形
- [15-layer-composition.md](api/15-layer-composition.md) — 层 / 合成
- [16-filters-effects.md](api/16-filters-effects.md) — 滤镜链
- [17-backend-cli.md](api/17-backend-cli.md) — 后端 / CLI
- [18-rough-style.md](api/18-rough-style.md) — 手绘粗糙风格渲染
- [19-blend-stroke.md](api/19-blend-stroke.md) — 对象级混合模式与描边对齐
- [20-edge-perturb.md](api/20-edge-perturb.md) — 多边形顶点扰动与圆角

---

## Skill Metadata

| Field | Value |
|-------|-------|
| Name | `pml` |
| Version | 1.2.0 |
| Language | PML (C++23 port, Skia backend) |
| Source | `G:\Project\PML\docs\SKILL.md` |
| API Docs | `docs/api/*.md` (19 files) |
| Updated | 2026-06-24 | (+edge-perturb: mode 17, API doc 20) |
| Author | PML Team |

### Related

- [AGENTS.md](../../pml-cpp/AGENTS.md) — C++ port architecture & build guide
- `docs/api/*.md` — Per-category API reference (signatures, params, examples)
- Python reference at `../pml/` — semantic ground truth for PML behavior
