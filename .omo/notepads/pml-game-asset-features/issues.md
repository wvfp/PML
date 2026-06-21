## Issues & Gotchas

### Open
- (none currently)

### Gotchas
- drawAtlas needs pre-rasterized tile images ¡ª architectural change from current per-object draw
- Normal map = replacement shading changes color at render time, not a new render pass
- Isometric needs Painter's Algorithm depth sorting (render back-to-front)
- Child shader support requires SkRuntimeEffect::ChildPtr
- Smoke test stubs must CHECK_ERROR when builtins don't exist yet (RED phase)
