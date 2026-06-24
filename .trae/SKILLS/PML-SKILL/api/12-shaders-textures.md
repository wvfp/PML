# 12 — SkSL Shaders + Noise + Multi-Texture

## SkSL 着色器

支持 SkSL（Skia Shading Language）着色器编译和应用。

| 函数 | 签名 | 说明 |
|------|------|------|
| `shader` | `(shader sksl-source)` | 编译 SkSL 源码，返回 shader handle（整数） |
| `apply-shader!` | `(apply-shader! graphic-object shader-handle)` | 对着色对象应用着色器，返回新 GraphicObject |
| `make-uniforms` | `(make-uniforms val1 val2 ...)` | 从数值列表创建 uniform 字节数据（返回字符串） |
| `apply-uniforms` | `(apply-uniforms shader-handle uniform-data-string)` | 将 uniform 数据绑定到着色器，返回新 handle |
| `uniform-float` | `(uniform-float shader-handle name value)` | 快捷设置单一 float uniform（name 当前被忽略） |

### 基本示例

```scheme
; 定义一个简单着色器
(define my-shader (shader "
uniform float u_time;
half4 main(float2 xy) {
  float r = sin(xy.x * 0.1 + u_time) * 0.5 + 0.5;
  float g = cos(xy.y * 0.1 + u_time) * 0.5 + 0.5;
  return half4(r, g, 0.75, 1.0);
}"))

; 方式一：make-uniforms + apply-uniforms（多个 uniform）
(define u (make-uniforms 1.5))                    ; 创建 4 字节 float 数据
(define s1 (apply-uniforms my-shader u))          ; 绑定到着色器 → 新 handle
(define result (apply-shader! (rect 0 0 200 200 :fill "white") s1))

; 方式二：uniform-float（快捷设置单个 uniform）
(define s2 (uniform-float my-shader "u_time" 1.5)) ; name 当前被实现忽略
(define result2 (apply-shader! (rect 0 0 200 200 :fill "white") s2))

(render *canvas* "shader.png")
```

---

## 噪声着色器

| 函数 | 签名 | 说明 |
|------|------|------|
| `noise-fractal` | `(noise-fractal [:freq-x F] [:freq-y F] [:octaves N] [:seed N] [:tile-width N] [:tile-height N])` | 分形布朗噪声 |
| `noise-turbulence` | `(noise-turbulence [:freq-x F] [:freq-y F] [:octaves N] [:seed N] [:tile-width N] [:tile-height N])` | 湍流噪声（绝对值变体） |
| `quantize-noise` | `(quantize-noise noise-shader :levels '((threshold "color") ...))` | 将噪声着色器量化为离散色阶，返回新 shader handle |

### 公共参数（noise-fractal / noise-turbulence）

| 参数 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `:freq-x` | float | 0.02 | X 方向基础频率 |
| `:freq-y` | float | 0.02 | Y 方向基础频率 |
| `:octaves` | int | 4 | 叠加层数（范围 1–255） |
| `:seed` | float | 0 | 随机种子 |
| `:tile-width` | int | 0 | 设置非零值开启无缝平铺 |
| `:tile-height` | int | 0 | 设置非零值开启无缝平铺 |

### quantize-noise 参数

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `noise-shader` | shader handle | 是 | 已创建的噪声着色器（来自 noise-fractal 或 noise-turbulence） |
| `:levels` | list | 是 | 色阶定义列表，每项为 `(threshold "color")`，threshold 0–1 浮点值，color 为 CSS 颜色字符串 |

色阶规则：从最小 threshold 开始匹配，噪声值 ≤ threshold 则输出对应的颜色。列表按 threshold 升序排列。

### 示例

```scheme
; 基本噪声
(define clouds (noise-fractal :seed 42 :octaves 6 :freq-x 0.04 :freq-y 0.04))
(apply-shader! clouds (rect 0 0 256 256 :fill "white"))
(render *canvas* "clouds.png")

; 无缝平铺噪声（256×256 贴图四方连续）
(define tile (noise-fractal :seed 0 :octaves 4 :freq-x 0.05 :freq-y 0.05
                            :tile-width 256 :tile-height 256))
(define tex (apply-shader! (rect 0 0 256 256 :fill "white") tile))
(render tex "seamless_noise.png")

; 四色阶石板 tile（无缝）
(define n (noise-fractal :seed 42 :octaves 6
                         :freq-x 0.04 :freq-y 0.04
                         :tile-width 64 :tile-height 64))
(define stone (quantize-noise n
  :levels '((0.25 "#2a2a2a") (0.50 "#555555") (0.75 "#8a8a8a") (1.0 "#bbbbbb"))))
(define tile (apply-shader! (rect 0 0 64 64 :fill "white") stone))
(render tile "stone_tile.png")

; 三色阶地形 tile（noise-turbulence + 不同颜色方案）
(define n2 (noise-turbulence :seed 99 :octaves 4
                             :freq-x 0.06 :freq-y 0.06
                             :tile-width 64 :tile-height 64))
(define terrain (quantize-noise n2
  :levels '((0.33 "#1a3a5c") (0.66 "#4a8c3f") (1.0 "#d4b483"))))
(define tile2 (apply-shader! (rect 0 0 64 64 :fill "white") terrain))
(render tile2 "terrain_tile.png")
```

### 无缝 tile 工作原理

着色器采样在 Canvas 坐标空间中进行。当 tile 被放置在 tilemap 中时，每个 tile 的 `with-transform (translate x y)` 修改了 Canvas 变换矩阵，使相邻 tile 的噪声采样坐标在边界处自然衔接：

- Tile 1 在 (0,0)：着色器采样 (0,0)~(64,64)
- Tile 2 在 (64,0)：Canvas CTM 偏移 translate(64,0)，着色器采样 (64,0)~(128,64)
- 设 `:tile-width N` 后 Skia 内部通过 `stitch()` 机制调整噪声频率为最接近的 `n/N`（n 为整数），使噪声晶格的周期与 tile 边界对齐，实现视觉无缝过渡
- `noise(N, y) != noise(0, y)` 是预期行为（不同晶格点随机梯度不同），但相邻像素间的变化率与 tile 内部一致（SeamDiff / InteriorDiff ≈ 1.2×），肉眼不可见拼缝
- 配合 `:tile-height` 类似，上下边界无缝

这保证了相邻图块拼接时**看不见拼接缝**。

---

## Voronoi（细胞）噪声

生成细胞/晶格噪声图案，类似生物细胞或马赛克瓷砖纹理。

| 函数 | 签名 | 说明 |
|------|------|------|
| `noise-voronoi` | `(noise-voronoi :cell-size N [:seed N] [:jitter F])` | Voronoi 细胞噪声，返回 shader handle（整数） |

### 参数

| 参数 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `:cell-size` | int | 必填 | 细胞大小（像素），控制单元格密度 |
| `:seed` | int | 0 | 随机种子，不同种子产生不同细胞分布 |
| `:jitter` | float | 0.5 | 细胞中心抖动幅度（0–1），0 为规则网格，1 为完全随机 |

### 原理

着色器在 3×3 邻域内搜索最近细胞中心，计算到最近中心的距离（F1）作为输出灰度值。
`jitter` 控制细胞中心从规则网格位置的偏移量——低 jitter 产生蜂窝/马赛克效果，
高 jitter 产生更有机的破碎细胞纹理。

### 示例

```scheme
; 基本 Voronoi（中等细胞 + 中等 jitter）
(define v (noise-voronoi :cell-size 32 :seed 42 :jitter 0.5))
(define tex (apply-shader! (rect 0 0 256 256 :fill "white") v))
(add tex)
(render "voronoi.png")

; 高 jitter 有机细胞
(define organic (noise-voronoi :cell-size 48 :seed 7 :jitter 0.9))
(apply-shader! (rect 0 0 256 256 :fill "white") organic)

; 搭配 quantize-noise 做色阶地形
(define v-terrain (noise-voronoi :cell-size 40 :seed 777 :jitter 0.7))
(define terrain (quantize-noise v-terrain
  :levels '((0.25 "#2d5a27") (0.50 "#6b8e23")
            (0.75 "#a0895c") (1.0  "#8b7355"))))
```

---

## 域扭曲（Domain Warp）

用一个噪声场扭曲另一个噪声的采样坐标，产生流动/扭曲的视觉效果。

| 函数 | 签名 | 说明 |
|------|------|------|
| `noise-warp` | `(noise-warp base-shader warp-shader :amount F [:freq F])` | 域扭曲，返回新 shader handle |

### 参数

| 参数 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `base-shader` | shader handle | 必填 | 被扭曲的基础噪声着色器 |
| `warp-shader` | shader handle | 必填 | 扭曲场着色器（用于生成偏移量） |
| `:amount` | float | 30.0 | 扭曲强度（像素偏移幅度） |
| `:freq` | float | 0.01 | 扭曲场采样频率——控制扭曲的波长 |

### 原理

`noise-warp` 编译一个 SkSL `compose_with_child_shaders` 组合着色器：
- `child_0` = base shader（被扭曲的噪声）
- `child_1` = warp field（扭曲场）
- 在 main() 中，先采样 warp field 得到偏移向量，然后偏移 base 的采样坐标

### 示例

```scheme
; 准备基础噪声和扭曲场
(define base (noise-fractal :octaves 6 :scale 0.025 :seed 1))
(define warp-field (noise-fractal :octaves 2 :scale 0.008 :seed 99))

; 柔和扭曲
(define mild (noise-warp base warp-field :amount 20.0 :freq 0.008))
(apply-shader! (rect 0 0 256 256 :fill "white") mild)

; 强扭曲
(define strong (noise-warp base warp-field :amount 80.0 :freq 0.005))
(apply-shader! (rect 0 0 256 256 :fill "white") strong)

; 用湍流噪声做扭曲场（产生破碎感）
(define rough (noise-turbulence :octaves 4 :scale 0.015 :seed 77))
(define turbulent (noise-warp base rough :amount 50.0 :freq 0.01))
```

---

## 噪声混合（Noise Blend）

在两个噪声着色器之间做渐变过渡混合。

| 函数 | 签名 | 说明 |
|------|------|------|
| `noise-blend` | `(noise-blend noise-a noise-b :mode M [:weight F])` | 两噪声混合，返回新 shader handle |

### 参数

| 参数 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `noise-a` | shader handle | 必填 | 第一个噪声着色器 |
| `noise-b` | shader handle | 必填 | 第二个噪声着色器 |
| `:mode` | keyword | 必填 | 过渡模式：`'gradient`（径向渐变）、`'horizontal`（水平过渡）、`'vertical`（垂直过渡） |
| `:weight` | float | 0.5 | 过渡位置（0–1），控制混合分界线位置 |

### 原理

`noise-blend` 编译一个 SkSL `compose_with_child_shaders` 组合着色器：
- `child_0` = noise A
- `child_1` = noise B
- 根据 mode 计算过渡因子 t，在 weight 周围 20% 宽度范围内做 smoothstep 过渡

### 示例

```scheme
(define noise-a (noise-voronoi :cell-size 24 :seed 0 :jitter 0.5))
(define noise-b (noise-fractal :octaves 5 :scale 0.03 :seed 42))

; 径向渐变混合
(define grad (noise-blend noise-a noise-b :mode 'gradient :weight 0.5))
(apply-shader! (rect 0 0 256 256 :fill "white") grad)

; 从左到右水平过渡
(define horiz (noise-blend noise-a noise-b :mode 'horizontal :weight 0.4))

; 从上到下垂直过渡
(define vert (noise-blend noise-a noise-b :mode 'vertical :weight 0.6))
```

---

## 多纹理着色器 (bind-textures)

绑定 Skia `uniform shader` child 纹理到 SkSL 着色器。

```scheme
(bind-textures shader-value
  :textures '((slot-name graphic-or-image) ...))
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `shader-value` | shader | 已编译的 SkSL 着色器（必填） |
| `:textures` | list | 纹理绑定列表（必填 kwarg） |

每个绑定项：
| 项 | 说明 |
|---|------|
| `slot-name` | symbol，对应 SkSL 中的 `uniform shader slot_name` |
| `graphic-or-image` | GraphicObject 或 image（渲染成纹理后绑定到插槽） |

### 示例：双纹理合成

```scheme
; 定义着色器
(define blend-shader (shader "
uniform shader base;
uniform shader overlay;
half4 main(float2 xy) {
  half4 b = base.eval(xy);
  half4 o = overlay.eval(xy);
  return half4(mix(b.rgb, o.rgb, o.a), b.a + o.a*(1-b.a));
}"))

; 绑定纹理 → 返回新 shader handle
(define with-tex (bind-textures blend-shader
  :textures '((base    (rect 0 0 128 128 :fill "red"))
              (overlay (circle 64 64 40 :fill "rgba(0,0,255,0.5)")))))

; 应用（注意参数顺序：先 GraphicObject，再 shader handle）
(define result (apply-shader! (rect 0 0 128 128 :fill "white") with-tex))
(render result "blend.png")
```

### 示例：组合位图纹理

```scheme
(define tex-shader (shader "
uniform shader tex;
uniform float u_alpha;
half4 main(float2 xy) {
  half4 c = tex.eval(xy);
  c.a *= u_alpha;
  return c;
}"))

; 绑定从文件加载的图像 → 返回新 shader handle
(define with-tex (bind-textures tex-shader
  :textures '((tex (image "background.png" 0 0 256 256)))))

; 设置 alpha → 返回新 shader handle
(define with-alpha (uniform-float with-tex "u_alpha" 0.8))

; 应用着色器（注意参数顺序：先 GraphicObject，再 shader handle）
(define result (apply-shader! (rect 0 0 256 256 :fill "white") with-alpha))
(render result "combined.png")
```

---

## 注意事项

- SkSL 语法类似 GLSL，类型用 `half4` / `float2` 等 Skia 类型
- `uniform shader` 声明子纹理插槽，用 `slot.eval(xy)` 采样
- 纹理绑定的 GraphicObject 会先在内部离屏渲染为 SkImage，再绑定为 child shader
- 不支持嵌入其他 PML 着色器的引用，仅支持 SkSL 代码字符串
- 着色器编译错误会返回带行号的错误信息
