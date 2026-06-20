# Draft: PML C++ Refactoring Design

**Objective:** Port the complete PML interpreter from Python 3.10+ to C++23, with a shader-based GPU rendering backend (replacing Pillow/Cairo), leveraging giflib for GIF export and stb_image for image loading, producing a standalone binary with equivalent functionality.

## Current Architecture (Python)

### Pipeline
Source → `Lexer.tokenize()` → `Parser.parse()` → `Expander.expand_all()` → `Evaluator.evaluate()` → Result

### Core Data Flow
```
Source Code → [Token Stream] → [AST (nested lists + atoms)] → [Expanded AST] → [Evaluated Value]
                                                ↑                               ↑
                                          stdlib/*.pml                    Environment (scope chain)
```

### Runtime Types (pml/types.py)
- `Expr = int | float | str | bool | None | list[Expr]`
- `Symbol(name)` - interned identifier
- `Keyword(name)` - keyword argument marker
- `Procedure(params, body, closure_env)` - user-defined closure
- `BuiltinProcedure(name, fn)` - native function
- `Macro(params, body, closure_env)` - non-hygienic macro

### Graphics Pipeline
```
GraphicObject[shape_type, params, fill/stroke, transform, children]
  → Canvas[width, height, objects[]]
  → RenderBackend (ABC)
    → PillowBackend.create_surface() → draw() → save_image()
```

### Animation
```
Animation[target_id, property, from/to, duration, easing]
  → Timeline[animations[], frame_hooks[]]
    → evaluate_at(t) → apply_modifications → render_frames → GIF
```

### Skeleton/IK
```
Joint[name, pos, length, angle, min/max_angle]
  → SkeletonTemplate[name, joints[]]
    → SkeletonInstance[x, y, angles[]]
      → solve_fabrik() / solve_ccd()
        → skin-bind: joint transforms → graphic parameters
```

### Module System
```
import "path.pml" as prefix
  → ModuleLoader[env, cache]
    → Module[name, env, exports[]]
      → prefix/get(symbol) → value
```

### External Dependencies
- Pillow (only graphics backend)
- MCP (Python `mcp` package for AI agent server)

## → 模块化的可插拔后端系统

### 核心设计: BackendRegistry

不再硬编码后端，而是通过 **BackendRegistry** 单例做运行时派发：

```cpp
// === src/pml/backend/backend.h ===
class RenderBackend {
public:
    virtual ~RenderBackend() = default;
    
    // Metadata
    virtual BackendInfo info() const = 0;
    bool has_capability(BackendCap cap) const;
    
    // Surface
    virtual auto create_surface(int w, int h, std::string_view bg)
        -> std::unique_ptr<Surface> = 0;
    
    // Drawing
    virtual void draw(Surface&, const GraphicObject&) = 0;
    
    // Output
    virtual void save_image(const Surface&, std::string_view path, 
                            std::string_view fmt) = 0;
    virtual void save_animation(std::span<const Surface*> frames,
                                std::string_view path, std::string_view fmt, int fps);
    
    // Shader (optional, check has_capability(Shaders))
    virtual auto compile_shader(std::string_view glsl)
        -> std::unique_ptr<Shader>;
    virtual void bind_shader_param(Shader&, std::string_view name, const Value&);
};

// === src/pml/backend/registry.h ===
class BackendRegistry {
public:
    using Factory = std::function<std::unique_ptr<RenderBackend>()>;
    
    static BackendRegistry& instance();
    
    // Registration (called by each backend's init)
    void add(std::string_view name, Factory, BackendInfo);
    
    // Selection
    auto create(std::string_view name) -> std::unique_ptr<RenderBackend>;
    auto create_best(BackendCap required) -> std::unique_ptr<RenderBackend>;
    
    // Query
    auto available() const -> std::vector<BackendInfo>;
    auto find(std::string_view name) const -> std::optional<BackendInfo>;
    
    // Active backend (used by render dispatch)
    void set_active(std::string_view name);
    auto& active() -> RenderBackend&;
    auto active_info() const -> BackendInfo;
};
```

### 每个后端就是一个文件 + 一行注册

```
src/pml/backend/
├── backend.h               # RenderBackend ABC
├── registry.h / registry.cpp  # BackendRegistry
├── capabilities.h          # BackendCap enum + BackendInfo
├── null_backend.cpp        # NullBackend (always compiled)
├── skia/
│   ├── skia_backend.cpp    # SkiaBackend : RenderBackend
│   └── CMakeLists.txt      # 可选的，通过 CMake 控制
├── cairo/
│   ├── cairo_backend.cpp
│   └── CMakeLists.txt
```

**添加一个新后端只需要**：
1. 继承 `RenderBackend`
2. 实现所有纯虚方法
3. 在 `.cpp` 底部调用 `BackendRegistry::instance().add("my-backend", ...)`

### 编译期选择 (CMake)

```cmake
# CMakeLists.txt
option(PML_BACKEND_SKIA   "Build Skia backend"   ON)
option(PML_BACKEND_CAIRO  "Build Cairo backend"   OFF)

if(PML_BACKEND_SKIA)
    add_subdirectory(skia)
endif()
if(PML_BACKEND_CAIRO)
    add_subdirectory(cairo)
endif()
```

编译进来的后端全部注册到 `BackendRegistry`，运行时任意切换。

### PML 层面的后端交互

```lisp
;; 查询可用后端
(list-backends)                → ("skia" "null")

;; 切换
(set-backend! "skia")          → switches to Skia

;; 查看当前
(current-backend)              → "skia"
(backend-capabilities)         → (:raster-cpu true :gpu true :shader true :gif true)

;; 按能力自动选择
(set-backend! (backend-with-capability :shader))  → auto-select best
```

### 测试专用: NullBackend

```cpp
class NullBackend : public RenderBackend {
    // 不渲染任何东西，只记录调用序列
    std::vector<RenderCall> calls_;
public:
    void draw(Surface&, const GraphicObject& obj) override {
        calls_.push_back({"draw", obj.shape_type, obj.params});
    }
    auto call_log() const -> const auto& { return calls_; }
    
    // 性能极快，适合 CI 验证"调用路径是否正确"
};
```

## → 渲染后端变更为 Skia (替代 Cairo)

### 为什么 Skia 比 Cairo 更适合

| 维度 | Cairo | Skia | Winner |
|------|-------|------|--------|
| **GPU 加速** | 纯 CPU 渲染 | 原生 GPU (Vulkan/GL/Metal) | **Skia** |
| **自定义 Shader** | 不支持 | `SkRuntimeEffect` — 运行时编译 GLSL | **Skia** |
| **SVG 路径** | 需手工转换 | `SkPath` 原生 + `SkParsePath` | **Skia** |
| **字体渲染** | fontconfig | `SkShaper` + fontconfig/DirectWrite | ≈ |
| **图像编码** | PNG/SVG/PDF | PNG/JPEG/WEBP 通过 `SkEncoders` | **Skia** (更多格式) |
| **依赖体积** | 较小 (cairo ~2MB) | 较大 (skia ~15MB) | Cairo |
| **vcpkg 可用** | ✅ | ✅ `skia` 已包含 | ≈ |
| **社区** | 通用 2D 库 | Chrome/Flutter/Android 核心 | **Skia** |

### 修订后的 Graphics 架构

```
                        ┌──────────────┐
                        │ GraphicObject │  ← 不变
                        └──────┬───────┘
                               │
                     ┌─────────▼─────────┐
                     │  RenderBackend     │  ← ABC 不变
                     │  (abstract class)  │
                     └─────────┬─────────┘
                               │
                     ┌─────────▼─────────┐
                     │   SkiaBackend      │  ← 替换 CairoBackend
                     │                    │
                     │  CPU Mode:         │
                     │  SkSurface_Raster  │
                     │                    │
                     │  GPU Mode:         │
                     │  GrContext+GL/     │
                     │  Vulkan/Metal      │
                     │                    │
                     │  Shader Mode:      │
                     │  SkRuntimeEffect   │  ← 新！
                     └─────────┬─────────┘
                               │
                    ┌──────────▼──────────┐
                    │    PML → SkShader   │  ← 新子系统
                    │  (shader DSL编译)    │
                    └─────────────────────┘
```

### PML Shader 子系统 (全新功能)

用户可以在 PML DSL 中直接编写和引用 shader：

```lisp
;; 方案 A: 直接嵌入 GLSL
(define gradient-shader
  (shader "
    uniform float2 u_res;
    half4 main(vec2 fragcoord) {
      return half4(fragcoord.x/u_res.x, fragcoord.y/u_res.y, 0, 1);
    }"))

;; 方案 B: PML 描述的 shader (编译到 SkRuntimeEffect)
(define plasma
  (pml-shader
    :type :fragment
    :param :time float
    :body (sin (+ (* fragcoord.x 0.01) time))))

;; 使用 shader
(add (rect 0 0 200 200 :fill gradient-shader))
```

### 对原计划的影响

| 原任务 | Cairo 版本 | Skia 版本 | 变更 |
|--------|------------|-----------|------|
| T12 | CairoBackend | **SkiaBackend** | 完全替换 |
| T14 | Color parsing | Color parsing | 不变 (Skia 颜色格式兼容) |
| — | — | **NEW: Shader 子系统** | 新增任务 |
| T26 | GIF via giflib | GIF via giflib | 不变 (读回 CPU) |
| T13 | Render dispatch | Render dispatch | 调整 (Skia API 适配) |
| vcpkg | cairo + giflib | **skia** + giflib | Cairo 移除 |

### Skia 的 vcpkg 依赖

```json
{
  "name": "pml-cpp",
  "version": "0.1.0",
  "dependencies": [
    "skia",
    "giflib",
    "nlohmann-json",
    "googletest"
  ]
}
```

注意：`skia` vcpkg port 已经内建了 `libpng`, `libjpeg-turbo`, `zlib`, `freetype`, `expat` 等依赖。不需要单独加 stb_image（Skia 自带图像解码）。

## Metis Review Findings

### 🔴 CRITICAL Issues
1. **GIF Export**: Cairo does NOT support GIF output. Need giflib or manual LZW implementation. Palette reduction logic (RGBA→palette→white background composite) must exactly match Pillow's.
2. **Image Loading**: Cairo only loads PNG natively (`cairo_image_surface_create_from_png`). Need stb_image for JPG/BMP/etc loading in `_draw_image`.
3. **Font Rendering**: Cairo (fontconfig+freetype) vs Pillow (`ImageFont.truetype`) have fundamentally different text rendering. Pixel-level matching is impossible. Need explicit decision on acceptable difference.
4. **`id()` based object identity**: Animation system uses Python's `id()` for GraphicObject tracking. C++ needs explicit `uint64_t id` field on GraphicObject.

### 🟡 HIGH Issues
1. **SVG Path Rendering**: Python converts SVG paths to polygons. Cairo has native path primitives. Two approaches diverge (polygon approximation vs bezier curves).
2. **MCP Protocol**: Need nlohmann/json for JSON-RPC over stdio. ~200 lines of C++ to implement.
3. **Error Type Mapping**: 12 PMLError subtypes → `std::variant<ErrorKind, SourceLoc>` with line/col/filename.
4. **Stdlib Embedding**: xxd -i for .pml files. Module path resolution needs adaptation for embedded files.
5. **Skin Binding + IK Integration**: Frame hooks call `ik-solve()` → skeleton state → `_merge_skin_bindings()` → modified GraphicObjects → render. Cross-cutting concern.

### 🟢 LOW Issues
1. **REPL**: linenoise or simple std::cin loop
2. **--watch mode**: Platform-specific file monitoring (inotify on Linux)
3. **Color parsing**: Straightforward mapping to Cairo's RGBA
4. **Performance baseline**: Not V1 concern, but document it

### Resolutions Applied
- GIF: Use giflib + implement palette reduction to match Pillow behavior
- Image loading: Use stb_image for non-PNG formats
- Font: Accept visual differences, document in BEHAVIOR_DIFFERENCES.md
- Object identity: Add `uint64_t id` field to GraphicObject + atomic counter

## Scope Boundaries

### IN SCOPE (V1 — ALL)
- ✅ Lexer → Parser → Expander → Evaluator full pipeline
- ✅ All PML runtime types: Symbol, Keyword, Procedure, BuiltinProcedure, Macro
- ✅ Environment with lexical scoping
- ✅ All builtins: arithmetic, comparison, IO, list/string ops, type predicates
- ✅ Module system: import/provide with caching and circular dependency detection
- ✅ Graphics pipeline: Canvas, GraphicObject, all primitives (circle/rect/ellipse/line/polygon/path/text/image)
- ✅ 2D AffineTransform (translate/rotate/scale/shear/compose/inverse)
- ✅ Cairo backend (replacing Pillow) including image loading via stb_image
- ✅ GIF animation export via giflib
- ✅ Animation system: Timeline, easing functions (12 types), property tweening, frame hooks
- ✅ Skeleton/IK system: Joint, SkeletonTemplate, SkeletonInstance, FABRIK + CCD solvers
- ✅ Sprite system: StyleDescriptor, Palette, component registry
- ✅ Stdlib embedding: compile all .pml files into binary, load at runtime
- ✅ CLI: file execution, REPL, --watch mode (inotify), --json output, -o output dir
- ✅ MCP server: JSON-RPC over stdio, 5 tools matching Python version
- ✅ Google Test suite: per-module tests (target: 389+ tests)

### OUT OF SCOPE (V1)
- ❌ No new DSL features or syntax changes
- ❌ No fixing existing Python bugs (document them)
- ❌ No SVG/PDF export (Cairo capability, but V1 stays with PNG/GIF)
- ❌ No MP4/webm video output
- ❌ No WebSocket/HTTP MCP transport (stdio only)
- ❌ No cross-compilation or CI pipeline (V2)
- ❌ No Windows/Mac CI testing (V1 targets Linux first)
- ❌ No C++ modules (std modules) — using traditional headers

### V2 CANDIDATES (tracked but not planned)
- Windows/macOS port
- SVG/PDF export
- WebSocket MCP transport
- Performance optimization pass
- CI pipeline with cross-platform testing

## Test Strategy
- **Framework**: Google Test
- **Workflow**: Tests-after-module — implement each module, then write corresponding tests
- **Coverage target**: 389+ tests matching Python test suite
- **Parity verification**: Python script running same .pml on both interpreters, comparing output hashes
- **Rendering QA**: Pixel-level comparison of PNG outputs between Python and C++ versions

## Key Design Decisions (for C++23)

### ✅ Decision Log
| # | Decision | Choice | Rationale |
|---|----------|--------|-----------|
| 1 | Build System | **CMake + vcpkg** | 主流方案，依赖管理成熟 |
| 2 | Graphics Backend | **Cairo** | 2D 矢量渲染能力全面，替代 Pillow |
| 3 | AST Type | **std::variant + unique_ptr** | 类型安全、现代 C++ |
| 4 | Error Handling | **std::expected<Value, Error>** | C++23 标准方案，无异常开销 |
| 5 | Testing | **Google Test** | 最成熟的 C++ 测试框架 |
| 6 | Directory | **深度分层 (deep hierarchy)** | 精细命名空间管理 |
| 7 | Stdlib | **编译期嵌入 + 运行时加载** | 无需外部文件依赖 |
| 8 | Repository | **独立 Git 仓库** | 分离开发、独立 CI |
| 9 | MCP Server | **C++ 重写** | 保持 AI agent 兼容 |
| 10 | Phasing | **自底向上** | Lexer → Parser → ... → CLI，每步可测试 |

### Proposed Directory Structure
```
pml-cpp/
├── CMakeLists.txt
├── vcpkg.json
├── README.md / LICENSE
├── src/
│   └── pml/
│       ├── core/             # types, error, environment
│       ├── frontend/         # lexer, parser, expander
│       ├── evaluator/        # evaluator, builtins, special forms
│       ├── module/           # module loader, import/provide
│       ├── graphics/         # canvas, objects, transform, render dispatch
│       ├── backend/          # cairo backend (RenderBackend impl)
│       ├── sprites/          # style, palette, registry
│       ├── animation/        # easing, timeline, interpolation
│       ├── skeleton/         # skeleton, ik_fabrik, ik_ccd
│       ├── cli/              # repl, main entry
│       └── mcp/              # MCP server
├── stdlib/                   # *.pml source files for embedding
├── tests/                    # Google Test (per-module)
└── examples/                 # .pml example files
```

### Key C++23 Features Used
- `std::expected<T, E>` - Error handling (C++23)
- `std::variant<...>` - Tagged union for Expr/Value types
- `std::optional<T>` - Optional values
- `std::span<const T>` - Non-owning array views
- `std::generator<T>` - Coroutine-based generators (if needed)
- `std::print` - Formatting output
- Concepts (`requires`) - Template constraints
- `std::unique_ptr<T>` / `std::shared_ptr<T>` - Ownership management
