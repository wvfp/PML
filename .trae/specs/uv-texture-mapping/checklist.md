# Checklist

## Build (优先级: 🔴 阻断)
- [ ] `cmake --build --preset debug` 全部目标链接成功
- [ ] `pml.exe` / `pml-builtins-smoke.exe` / `pml-mcp.exe` 全部产出
- [ ] `Value::as_graphic_object` 等新增 `as_*` 访问器在 `pml_core` 中正确导出
- [ ] `SkVertices::MakeCopy` 参数顺序与 Skia 头文件一致(避免 C2665)

## Foundation — Wave 1 (基础类型与缓存)
- [ ] `core/texture.h::TextureBox` 字段完整:`stable_id`/`go`/`width`/`height`/`wrap_x`/`wrap_y`/`filter`
- [ ] `core/texture_cache.h/cpp` 提供 LRU 缓存,`TextureCache::instance()` 委托给 `PMLContext::current()`
- [ ] `PMLContext` 含 `std::unique_ptr<TextureCache> texture_cache;`
- [ ] `core/types.h::Box::Kind::Texture` 存在
- [ ] `Value::as_texture()` / `is_texture()` 存在
- [ ] `value_to_string` 处理 `Box::Kind::Texture` 时不崩溃
- [ ] `graphics/params.h` 新增 7 项 `ParamKey`:`uv`/`uv_mode`/`wrap_x`/`wrap_y`/`filter`/`uv_vertices`/`edge_blend`
- [ ] `static_assert(count <= 32)` 仍然通过
- [ ] `graphics/cg_solver.h` 提供 `SparseMatrix`/`CGSolver`/内联自测

## Algorithms — Wave 2
- [ ] `graphics/triangulation.h/cpp` 耳剪法三角化支持 `polygon`/`path`/`rect`/`ellipse`/`circle`
- [ ] CCW 缠绕校正,空/退化轮廓不崩溃
- [ ] `uv_solver::solve_planar_uv` 把 AABB 映射到 `[0,1]²`,零宽/高归一化到 1.0
- [ ] `uv_solver::solve_harmonic_uv` 通过 CG 求解,边界 UV 在 `[0,1]²` 边界上
- [ ] `uv_solver::apply_explicit_uv` 用 `:uv-vertices` 列表;不足时回退到 Planar
- [ ] `texture_bake::bake_texture` 在非 Skia 构建下返回 `nullptr`

## Skia Backend — Wave 3
- [ ] `backend/skia/textured_draw.cpp::bake_graphic_object_to_skimage` 渲染到 `SkSurface` 再 `makeImageSnapshot`
- [ ] `draw_textured_shape` 调用三角化 → UV 求解 → 烘焙 → `SkVertices::MakeCopy` → `canvas->drawVertices`
- [ ] 描边保持纯色单独通道(在纹理填充之上)
- [ ] `skia_backend_draw.cpp::draw_object` 优先判断 `ParamKey::uv`,命中则路由到 `draw_textured_shape`
- [ ] 未命中 UV 路径时,旧行为完全一致(像素级一致)

## Builtins — Wave 3
- [ ] `define-texture name (width height) body` 接受 int/double 尺寸
- [ ] `texture?` / `texture-width` / `texture-height` / `texture-id` 注册并工作
- [ ] `texture-map` 接受 7 个 kwargs:`:source`/`:mode`/`:wrap-x`/`:wrap-y`/`:filter`/`:uv-vertices`/`:edge-blend`
- [ ] `texture-map` 在返回的 `GraphicObject` 上设置 `ParamKey::uv` 与 `uv_mode`
- [ ] 缺少 `:source` 时抛类型错误(而非崩溃)

## Integration — Wave 4
- [ ] `api/api.cpp::PMLRuntime::init_global_env` 调用 `register_texture_builtins` 与 `register_texture_map_builtins`
- [ ] `core/CMakeLists.txt` 包含 `texture_cache.cpp`
- [ ] `graphics/CMakeLists.txt` 包含 `triangulation.cpp`/`uv_solver.cpp`/`texture_bake.cpp`
- [ ] `evaluator/CMakeLists.txt` 包含 `texture_builtins.cpp`/`texture_map_builtins.cpp`
- [ ] `backend/skia/CMakeLists.txt` 包含 `textured_draw.cpp`
- [ ] 纹理值通过 `provide`/`import` 时保持不变(Box::Kind::Texture)

## Tests — Wave 4
- [ ] `define-texture` 创建有效纹理的烟雾测试
- [ ] `texture-map :source tex` 设置 UV 参数的测试
- [ ] 缺少 `:source` 的 `texture-map` 抛类型错误测试
- [ ] 平面 UV 单元方块四角映射测试
- [ ] CG 求解器自测通过
- [ ] 已存在的 134 个烟雾测试不发生回归

## Examples — Wave 4
- [ ] `examples/texture-basic.pml` 运行并产出 PNG
- [ ] `examples/texture-starry-sky.pml` 运行并产出 PNG
- [ ] `examples/texture-repeat.pml` 运行并产出 PNG
- [ ] `examples/texture-explicit.pml` 运行并产出 PNG

## Verification (最终关卡)
- [ ] `pml-builtins-smoke.exe` 退出码 0
- [ ] `pml.exe examples/texture-basic.pml -o out\basic` 产生非空 PNG
- [ ] `pml.exe examples/texture-starry-sky.pml -o out\sky` 产生非空 PNG
- [ ] `pml.exe examples/texture-repeat.pml -o out\repeat` 产生非空 PNG
- [ ] `pml.exe examples/texture-explicit.pml -o out\explicit` 产生非空 PNG
- [ ] 已存在的烟雾测试 0 回归

## Guardrails (🛑 强制禁项)
- [ ] 无 LSCM / Cage / Eigen 代码
- [ ] 无文件级纹理约定
- [ ] 无纹理化描边(描边仍为纯色)
- [ ] 无 UV 动画
- [ ] 无多图层纹理编辑器

## Delivery
- [ ] 所有 Wave 1-4 源码就位
- [ ] `git commit` 在 `dev-cpp` 分支
- [ ] `git push origin dev-cpp` 成功
