# Texture/Shader 系统现状评估与扩展方向

## 一、当前已有哪些

### Shader 管线（`shader_builtins.cpp`）

```
(shader "sksl")                          → 编译 SkSL → handle
(apply-shader! go handle [:coordinate-space :world] [:blend-mode ...])
                                         → 把 shader 挂到 GraphicObject
(make-uniforms float ...)                → 打包 float 为 uniform 字节串
(apply-uniforms handle uniform-bytes)    → 创建带 uniform 的新 shader handle
(uniform-float handle name value)        → 设单个 uniform（⚠️ 多 uniform 时失效）
```

### 噪声 Shader（`shader_builtins.cpp`）

```
(noise-fractal :freq-x :freq-y :octaves :seed :tile-w :tile-h
               :lacunarity :persistence)        → Perlin 分形噪声
(noise-turbulence ...)                           → Perlin 湍流噪声
(noise-voronoi :cell-size :seed :jitter)         → 沃罗诺伊噪声
(noise-warp base warp :amount :freq)             → 域扭曲（内部用 compose_with_child_shaders）
(noise-blend a b :mode :direction :weight)       → 混合两个噪声（内部用 compose_with_child_shaders）
(quantize-noise handle :levels '((thresh color) ...)) → 噪声量化（内部用 compose_with_child_shader）
```

### 纹理输入（`multi_texture_builtins.cpp`）

```
(bind-textures handle :textures '((slot-name graphic-obj) ...))
    → GO → 光栅化到 SkSurface → snapshot SkImage → makeShader
    → 绑定到 SkSL 的 uniform shader <slot-name> 插槽
```

### 外部图片

```
(load-image "path.png" :x :y :width :height ...) → 返回 GraphicObject
    → 这个 GO 可以做 bind-textures 的输入
    → 但走的是光栅化路径（GO → 像素 → shader child）
```

---

## 二、成功案例给我们的启示

### 案例 1：双网格瓦片系统（当前项目）

**做法**：全部内联在单个 SkSL 字符串中，6 个 uniform 通过 `make-uniforms` + `apply-uniforms` 传递

**痛点**：
- 64 次调用 `apply-uniforms`，每次编译/缓存一份新 shader
- SkSL 中的 FBM/SDF/terrain_color 都是重复代码（每个瓦片都重新编译）
- 没有复用预编译好的地形着色器

**如果有了 compose-with-child，理想写法是**：

```pml
;; 编译一次
(define terrain-sksl (shader "uniform shader src; uniform float u_type; half4 main(float2 xy) { ... }"))
(define sdf-sksl (shader "uniform shader txtr; uniform float u_bl,u_br,u_tl,u_tr; half4 main(float2 xy) { ... }"))

;; 每个瓦片：只传 uniform，不重新编译
(define terrain (apply-uniforms terrain-sksl (make-uniforms type-id seed)))
(define composed (compose-with-child sdf-sksl terrain))
(define final-s (apply-uniforms composed (make-uniforms bl br tl tr seed)))
```

### 案例 2：量化噪声（已内置）

`quantize-noise` 是 compose-with-child 的成功案例模板。它：
1. 编译一次噪声 shader（由 PML builtin 产生）
2. 动态生成量化 wrapper SkSL（依赖用户提供的色标）
3. **零光栅化**：`backend.compose_with_child_shader(noise_handle, wrapper_sk)`

这证明了 compose-with-child 的有效性和稳定性。

### 案例 3：域扭曲（已内置）

`noise-warp` 更进一步，使用 `compose_with_child_shaders` 同时绑定 **两个** child shader + uniform 数据，在 GPU 端构建完整管线。

---

## 三、建议新增功能

### 🎯 P0 — 最容易、最自然、影响最大

#### 1. 暴露 `compose-with-child` / `compose-with-children`

直接把后端方法绑到 PML 层。已有成功案例（quantize-noise, noise-warp, noise-blend 都在内部用），成本极低。

```pml
;; API 设计
(compose-with-child wrapper-sksl child-handle)
(compose-with-children wrapper-sksl (list child-0 child-1) :uniforms (make-uniforms ...))
```

**预期效果**：shader 模块化、复用编译结果、零像素落地。

---

### 🎯 P1 — 开发者体验

#### 2. `shader-uniforms` — 查看编译后的 uniform 信息

```pml
(shader-uniforms sksl-handle)
;; → ((u_bl float) (u_br float) (u_seed float) (u_type float))
```

当前 `uniform-float` 的 `name` 参数被注释为 `// Ignore name for now (would need shader reflection)`，说明已经意识到需要这个能力。Skia 的 `SkRuntimeEffect` 提供了 `uniforms()` 方法返回 uniform 列表（名称、类型、数量），只是没暴露。

**为什么有用**：调试 uniform 顺序/大小问题将变为可视化，不再需要猜。

#### 3. `shader-children` — 查看 child shader 插槽

```pml
(shader-children sksl-handle)
;; → ("tex" "mask")
```

对应 `SkRuntimeEffect::children()`，对 `bind-textures` / `compose-with-child` 调试有帮助。

#### 4. `eval-shader` — 在 CPU 端评估 shader 采样

```pml
(eval-shader handle x y)
;; → (r g b a)
```

将 shader handle 渲染到一个 1×1 的 SkSurface 上采样颜色返回。等价于在 shader main 里 `return` 一个值看看结果，解决"着色器编译成功但输出全黑"的调试困境。

---

### 🎯 P2 — 纹理输入增强

#### 5. `image-shader` — 把加载的图片直接变成 child shader

当前加载外部图片只能通过 `(load-image "path.png")` 得到一个 GraphicObject，然后：
- 作为矩形用 `add` 显示
- 或者通过 `bind-textures` 走光栅化路径

如果能直接把图片转为 shader child，就省掉了光栅化那一步：

```pml
(define img (image-shader "path.png" :tile-mode 'repeat))
;; 返回一个 shader handle，可以直接用在 bind-textures 或 compose-with-child 的 uniform shader 插槽
```

**内部实现**：`load-image` 现有的解码逻辑不变，但结果是 `SkImage::makeShader(...)` 而不是 `GraphicObject`。进入 `preshader_cache_` 而不是 `shader_cache_`。

**参考引擎**：Unity 的 `Material.SetTexture()`、Godot 的 `ShaderMaterial` 都支持直接绑定纹理。

---

### 🎯 P3 — 进阶能力

#### 6. `(sksl-type-size "float2")` / `(sksl-struct-size "struct { float2 a; float b; }")` — SkSL 内存布局辅助

`make-uniforms` 目前只打包 `float`（4 字节），但 SkSL 支持 `float2`、`float3`、`float4` 甚至 struct。如果用户想要在 uniform 中传递 `float2`，需要手动填充 8 字节。提供计算类型大小的辅助函数可以减少错误。

#### 7. `canvas->shader` — 把当前 canvas 内容转为 shader

在后处理场景中很有用：渲染场景到 canvas，然后捕获为 shader 做全屏模糊、辉光等。

#### 8. Uniform 动画

```pml
;; timeline + uniform 联动
(timeline-animate-uniform shader-handle "u_amount" 0.0 100.0 2000)
```

PML 已经有动画系统（`pml_animation`），连接到 uniform 可以实现 shader 参数随时间变化。

---

### 🎯 总结优先级矩阵

| 功能 | 实现成本 | 开发效率提升 | 表达力提升 | 优先级 |
|------|---------|------------|-----------|--------|
| compose-with-child | 低（1 行注册已有 backend 方法） | 高 | 高 | **P0** |
| compose-with-children | 低（同上 + uniform 透传） | 高 | 高 | **P0** |
| shader-uniforms 内省 | 低（`effect->uniforms()` 已可调） | 高（调试效率） | 中 | **P1** |
| shader-children 内省 | 低 | 中 | 中 | **P1** |
| eval-shader | 低（1×1 渲染采样） | 高（调试效率） | 中 | **P1** |
| image-shader | 中（复用 load-image 解码 + makeShader） | 中 | 高（纹理管线完整） | **P2** |
| canvas→shader | 高（涉及 canvas state 管理） | — | 高（后处理能力） | **P3** |
| Uniform 动画 | 中（动画系统已存在） | — | 高 | **P3** |

### 与你的双网格项目的直接关联

| 功能 | 能帮你做什么 |
|------|-------------|
| **compose-with-child** | 地形着色器编译一次，64 个瓦片只传 uniform |
| **shader-uniforms** | 调试 `make-uniforms` 顺序/大小 |
| **eval-shader** | 验证单个瓦片的 SDF 输出是否正确 |
| **image-shader** | 加载真实地形纹理作为背景（而非纯程序化） |
| **canvas→shader** | 将第一次渲染输出作为后处理的输入 |
