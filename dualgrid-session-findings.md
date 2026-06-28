# Dual-Grid 瓦片实现 — 会话总结（2026-06-27）

本文件整理了实现 Dual-Grid 瓦片系统过程中所产生的 PML 优化反馈和纹理/着色器管线评估。源文件见 `examples/dualgrid-tileset/`。

---

## 一、PML 优化问题（9 项）

来源于 Dual-Grid 瓦片系统的实现经验，按优先级排列。详见 `examples/dualgrid-tileset/pml-issues.md`。

### P0 — 阻断性

| 问题 | 根因 | 建议 |
|------|------|------|
| `uniform-float` 多 uniform 传递失败 | 每次调用创建只带 4 字节的新 shader，SkSL 声明 6×float 需 24 字节 | 重命名/重构 `uniform-float`；`apply-uniforms` 兼容单值；uniform 大小不匹配时给出可读错误 |
| 字符串括号匹配错误行号偏移 | Lexer 将字符串字面量内部的 `)` 计入括号计数 | Lexer 扫描字符串时跳过括号计数；或引入多行字符串语法 `(shader #[...#])` |

### P1 — 语言设计

| 问题 | 建议 |
|------|------|
| `div` 不存在，需用 `quotient` | 添加 `div` 作为别名，或 `unbound` 错误提示 `did you mean: quotient?` |
| `to-double` 不存在，无法显式控制 Int/Double | 添加 `->double`/`->int` 转换函数 |
| `dotimes` 不可用，需 `for-each`+`range` 三层嵌套 | 实现 `dotimes` 宏或内建函数 |

### P2 — 调试诊断

| 问题 | 建议 |
|------|------|
| Skia uniform 绑定失败无诊断信息 | 捕获 Skia 错误后补充：期望/实际数据大小、uniform 名称列表、数据布局 |
| Shader 无运行时调试手段 | 添加 `shader-uniforms` 反射、`shader-validate` 预检查 |

### P3 — 文档

- `make-uniforms`+`apply-uniforms` API 无文档、不可发现
- PML 类型系统隐式转换规则未文档化

---

## 二、Texture/Shader 系统现状与增强计划

详见 `examples/dualgrid-tileset/texture-shader-enhance.md`。

### 当前已有能力

- **Shader 管线**: `shader` → `apply-shader!` / `make-uniforms` + `apply-uniforms` / `uniform-float`
- **噪声 Shader**: `noise-fractal` / `noise-turbulence` / `noise-voronoi` / `noise-warp` / `noise-blend` / `quantize-noise`
- **纹理输入**: `bind-textures`（GO 光栅化 → SkImage → uniform shader 插槽）
- **外部图片**: `load-image`（返回 GO，可走 `bind-textures` 光栅化路径）

### 增强计划（8 项）

| 优先级 | 功能 | 实现成本 | 说明 |
|--------|------|---------|------|
| **P0** | `compose-with-child` / `compose-with-children` | 低 | 后端已有 `compose_with_child_shader`，quantize-noise / noise-warp / noise-blend 内部已使用，只需暴露 PML 绑定。实现 shader 模块化、复用编译、零像素落地 |
| **P1** | `shader-uniforms` 内省 | 低 | `SkRuntimeEffect::uniforms()` 已可调，调试 uniform 顺序/大小可视化 |
| **P1** | `shader-children` 内省 | 低 | `SkRuntimeEffect::children()` 已可调 |
| **P1** | `eval-shader` | 低 | 1×1 渲染采样 shader 输出颜色，解决"全黑"调试 |
| **P2** | `image-shader` | 中 | 解码 PNG 直接 `SkImage::makeShader`，不走光栅化 |
| **P3** | sksl 类型尺寸辅助 | 低 | float2/float3/float4 内存布局计算 |
| **P3** | canvas→shader | 高 | canvas 内容转为 shader，后处理用 |
| **P3** | uniform 动画 | 中 | timeline 驱动 shader 参数 |

### 成功案例

1. **Dual-Grid 瓦片（当前项目）** — 全部内联单 SkSL，64 次 `apply-uniforms` 重复编译 → 说明需要 `compose-with-child` 实现模块化
2. **Quantize-noise（已内置）** — compose-with-child 的成功模板：编译一次噪声 → 动态生成 wrapper → 零光栅化组合
3. **Noise-warp（已内置）** — 使用 `compose_with_child_shaders` 同时绑定两个 child shader + uniform

### Compose-With-Child API 设计建议

```pml
;; 单 child（SkSL 声明 uniform shader src;）
(compose-with-child wrapper-sksl child-shader-handle)

;; 多 child + uniforms
(compose-with-children wrapper-sksl (list child-0 child-1) :uniforms (make-uniforms ...))
```

后端位置：`skia_backend.cpp` line ~794 (`compose_with_child_shader`) 和 line ~843 (`compose_with_child_shaders`)。暴露只需在 `shader_builtins.cpp` 加 PML 绑定注册。

---

## 三、架构关键发现

- **双缓存结构**：Skia 后端有 `preshader_cache_`（纯 shader，用于 compose）和 `shader_cache_`（绑定后用于渲染），由不同函数写入
- **`bind-textures` 的限制**：GO → 光栅化到 SkImage → makeShader，分辨率锁定、有像素化问题
- **`compose_with_child_shader` 的优势**：直接在 GPU 端组合 shader 管线，无像素落地，已用于内置噪声功能
- **`load-image` 缺口**：返回 GraphicObject 而非 shader child，外部图片进 shader 管线必须走光栅化

---

## 文件索引

| 文件 | 内容 |
|------|------|
| `examples/dualgrid-tileset/main.pml` | 完整实现源码（自包含 SkSL + 3 个渲染输出） |
| `examples/dualgrid-tileset/README.md` | 实现流程文档（架构、SkSL 核心、双网格算法详解） |
| `examples/dualgrid-tileset/pml-issues.md` | 9 项 PML 优化问题（P0-P3） |
| `examples/dualgrid-tileset/texture-shader-enhance.md` | 纹理/着色器系统评估 + 8 项增强计划 |
