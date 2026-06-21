# Draft: PML 游戏素材增强 — Tilemap + 预烘焙 + SkSL 多纹理

## Core Objective
为 PML 扩展三条管线，使游戏美术师能够用 PML 代码定义和渲染多层 tilemap、导出多通道精灵素材（albedo/normal/specular），并通过 SkSL 多纹理着色器实现动态光照——全部通过 PML 内置函数控制，0 行运行时 C++ 代码。

## Requirements (confirmed)

- **Tilemap 类型**: C++ lightweight Tilemap data structure + Tileset + Orthogonal/Isometric batch rendering
  - PML API 风格: 矩阵式 (define-tileset, make-tilemap, tilemap-set!, render-tilemap)
  - 正交批渲染: drawAtlas (预栅格化瓦片到纹理图集)
  - 等距批渲染: drawVertices (3面钻石瓦片 + 深度排序)
  - 多层支持: ground + decoration 至少2层
- **Pre-baking (render-channels)**: Multi-channel sprite export
  - 通道: albedo + normal + specular
  - 法线生成: 替换着色法 (渲染时把 fill color 替换为法线向量颜色)
- **SkSL 多纹理**: Extend shader pipeline for `uniform shader` child textures
  - 新增 bind-textures builtin
  - 修改 RenderBackend 接口支持 child shaders
- **测试策略**: TDD (先写测试再实现)
- **投影模式**: 正交 + 等距都做

## Technical Decisions

- Tilemap 数据结构: 一维 int 数组 (flat array), 0=empty, 正数=tileset index
- Tileset: 缓存每种 tile type 的 GraphicObject 模板, drawAtlas 前预栅格化为 SkImage
- 法线贴图: 遍历 GraphicObject 树, 将 fill/stroke 颜色替换为法线向量 RGB
- 高光贴图: 灰阶输出 (single channel 或 RGB 重复)
- 每个特性独立文件集, 无跨文件依赖
- RenderBackend 新增虚方法 create_shader_with_children() (null backend 返回错误)

## Metis Findings

- drawAtlas 需要预栅格化 tilet type 到 SkImage 纹理图集, 这是架构变化
- 法线贴图生成方式决定 render-channels 实现路径
- 三个特性可以独立并行开发
- 需要严格的 scope guardrails (无碰撞、无物理、无材料系统、无 HDR)
- 现有 134/134 smoke tests 不能破坏

## Open Questions

- (全部已解决)

## Scope Boundaries

- INCLUDE: Tilemap 数据结构 + Tileset + Orthogonal render + Isometric render + render-channels (albedo/normal/specular) + SkSL 多纹理 (bind-textures)
- EXCLUDE: 碰撞层、物理、路径寻路、材料系统、HDR、延迟渲染、运行时瓦片图编辑、自动图块化、精灵表打包器复用
