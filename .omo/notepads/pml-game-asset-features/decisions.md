## Decisions

| Date | Decision | Rationale |
|------|----------|-----------|
| 2026-06-21 | Tilemap API: 矩阵式 (define-tileset → make-tilemap → tilemap-set! → render-tilemap) | 用户确认 |
| 2026-06-21 | 正交批渲染: drawAtlas | 用户确认 |
| 2026-06-21 | 法线贴图: 替换着色法 (fill color → 法线向量 RGB) | 用户确认 |
| 2026-06-21 | 测试: TDD (先写 smoke test stubs，再实现) | 用户确认 |
| 2026-06-21 | BackendCap → uint16_t for Tilemap + RenderChannels flags | 8 bits exhausted |
| 2026-06-21 | RenderBackend add create_shader_with_children() virtual | Metis recommendation |
