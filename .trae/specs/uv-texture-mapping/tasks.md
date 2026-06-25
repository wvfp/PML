# Tasks

> 该计划基于 `.omo/plans/uv-texture-mapping.md`。当前 Wave 1-3 的核心代码已实现,
> 仍有几处构建错误需修复、补全烟雾测试与示例,并提交推送。
> 所有代码位于 `g:\Project\PML\src\pml/...`,计划 ID = `uv-texture-mapping`。

- [ ] Task 1: 修复 `pml-builtins-smoke` 与 `pml-mcp` 的链接错误
  - [ ] SubTask 1.1: 排查 `Value::as_graphic_object` 链接错误 — 重新构建 `pml_core` 库,确认 `core/types.cpp` 已被重新编译并导出该符号(`cmake --build --preset debug --target rebuild_cache && cmake --build --preset debug --target pml_core`)
  - [ ] SubTask 1.2: 修复 `SkVertices::MakeCopy` 参数不匹配 — 将 `textured_draw.cpp` 中的 `uint32_t` 索引改为 `uint16_t`(若顶点数 > 65535 则改用 `SkVertices::Builder`)
  - [ ] SubTask 1.3: 运行 `cmake --build --preset debug` 端到端,确认 `pml.exe`、`pml-builtins-smoke.exe`、`pml-mcp.exe` 全部链接成功

- [ ] Task 2: 验证 Wave 1 基础组件(已实现,需核对)
  - [ ] SubTask 2.1: `core/texture.h` 含 `TextureBox { stable_id, go, width, height, wrap_x, wrap_y, filter }`,`atomic` 计数器自增
  - [ ] SubTask 2.2: `core/texture_cache.h/cpp` 实现 LRU 缓存,`instance()` 委托给 `PMLContext::current()`
  - [ ] SubTask 2.3: `core/types.h` 含 `Box::Kind::Texture`,`Value::as_texture()` 与 `is_texture()` 存在;`value_to_string` 处理 Texture
  - [ ] SubTask 2.4: `graphics/params.h` 含 7 个新 `ParamKey`:`uv`/`uv_mode`/`wrap_x`/`wrap_y`/`filter`/`uv_vertices`/`edge_blend`
  - [ ] SubTask 2.5: `graphics/cg_solver.h` 是 header-only,内联自测 `test_cg_solver_all()` 可用

- [ ] Task 3: 验证 Wave 2 核心算法
  - [ ] SubTask 3.1: `graphics/triangulation.h/cpp` 耳剪法三角化支持 `polygon`/`path`/`rect`/`ellipse`/`circle`
  - [ ] SubTask 3.2: `graphics/uv_solver.h/cpp` 暴露 `solve_planar_uv`/`solve_harmonic_uv`/`apply_explicit_uv`
  - [ ] SubTask 3.3: `graphics/texture_bake.h/cpp` 在非 Skia 构建下返回 `nullptr`,Skia 下委托 `bake_graphic_object_to_skimage`

- [ ] Task 4: 验证 Wave 3 Skia 后端与 PML 内置
  - [ ] SubTask 4.1: `backend/skia/textured_draw.cpp` 实现 `bake_graphic_object_to_skimage` 与 `draw_textured_shape`(用 `SkVertices::MakeCopy` + `drawVertices`)
  - [ ] SubTask 4.2: `backend/skia/skia_backend_draw.cpp::draw_object` 检查 `ParamKey::uv` 并路由到 `draw_textured_shape`,描边保持纯色单独通道
  - [ ] SubTask 4.3: `evaluator/texture_builtins.h/cpp` 注册 `define-texture`/`texture?`/`texture-width`/`texture-height`/`texture-id`
  - [ ] SubTask 4.4: `evaluator/texture_map_builtins.h/cpp` 注册 `texture-map`,支持 `:source`/`:mode`/`:wrap-x`/`:wrap-y`/`:filter`/`:uv-vertices`/`:edge-blend`

- [ ] Task 5: 验证 Wave 4 集成
  - [ ] SubTask 5.1: `api/api.cpp::PMLRuntime::init_global_env` 调用 `register_texture_builtins` 与 `register_texture_map_builtins`
  - [ ] SubTask 5.2: `core/CMakeLists.txt` 含 `texture_cache.cpp`、`types.cpp`
  - [ ] SubTask 5.3: `graphics/CMakeLists.txt` 含 `triangulation.cpp`/`uv_solver.cpp`/`texture_bake.cpp`
  - [ ] SubTask 5.4: `evaluator/CMakeLists.txt` 含 `texture_builtins.cpp`/`texture_map_builtins.cpp`
  - [ ] SubTask 5.5: `backend/skia/CMakeLists.txt` 含 `textured_draw.cpp`
  - [ ] SubTask 5.6: 纹理值通过 `provide`/`import` 时无需特殊序列化(Box::Kind::Texture 是一等值)

- [ ] Task 6: 在 `tests/builtins_smoke.cpp` 添加 UV 纹理相关烟雾测试
  - [ ] SubTask 6.1: `define-texture` 创建有效纹理(`texture?`/`texture-width`/`texture-height` 断言)
  - [ ] SubTask 6.2: `texture-map :source tex` 在形状上设置 UV 参数
  - [ ] SubTask 6.3: 缺少 `:source` 的 `texture-map` 抛类型错误
  - [ ] SubTask 6.4: 平面 UV 单元方块四角映射到 `(0,0)`/`(1,0)`/`(1,1)`/`(0,1)`
  - [ ] SubTask 6.5: CG 求解器自测通过(3×3 对角、4×4 Laplacian)

- [ ] Task 7: 在 `examples/` 下添加 4 个示例 `.pml` 文件
  - [ ] SubTask 7.1: `examples/texture-basic.pml` — 红色矩形纹理映射到矩形
  - [ ] SubTask 7.2: `examples/texture-starry-sky.pml` — 复合多形状纹理(星星+月亮)映射到多边形
  - [ ] SubTask 7.3: `examples/texture-repeat.pml` — 演示 `:wrap-x :repeat`/`:wrap-y :repeat`
  - [ ] SubTask 7.4: `examples/texture-explicit.pml` — 用 `:uv-vertices` 手工控制 UV

- [ ] Task 8: 运行烟雾测试并视觉验证
  - [ ] SubTask 8.1: `cmake --build --preset debug` 编译成功
  - [ ] SubTask 8.2: 运行 `build\debug\tests\Debug\pml-builtins-smoke.exe`,退出码 0
  - [ ] SubTask 8.3: `build\debug\src\pml\cli\Debug\pml.exe examples\texture-basic.pml -o out\basic` 生成非空 PNG
  - [ ] SubTask 8.4: 同样跑 `texture-starry-sky.pml` / `texture-repeat.pml` / `texture-explicit.pml`

- [ ] Task 9: 提交并推送到 `dev-cpp` 分支
  - [ ] SubTask 9.1: `git add` 新增与修改的文件
  - [ ] SubTask 9.2: `git commit` 用约定式消息(参考:`feat(graphics): UV texture mapping system (define-texture, texture-map, planar/harmonic/explicit)`)
  - [ ] SubTask 9.3: `git push origin dev-cpp`

# Task Dependencies

- Task 1 → Task 2-9(必须先修好构建错误)
- Task 2-4(核对基础/算法/内置)→ Task 5
- Task 5(注册 + CMake)→ Task 6(测试要能注册到运行时)
- Task 6 → Task 8.2(测试要先编译通过)
- Task 7 → Task 8.3 / 8.4(示例要先写好)
- Task 8 → Task 9(验证通过后再推送)
