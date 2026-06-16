# Decisions - PML C++ Refactor

Initialized: 2026-06-16

## Architecture Decisions
1. Backend: Skia (GPU/shader) — replaces Cairo + stb_image
2. BackendRegistry: compile-time registration, runtime switching
3. NullBackend: always compiled, records calls for testing
4. Shader: SkRuntimeEffect fragment shaders only
5. Stdlib: compile-time xxd embedding
6. Tests: Google Test, tests-after-module
