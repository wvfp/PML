# PML C++23 Refactor Plan

## TL;DR

> **Objective**: Port the complete PML interpreter from Python 3.10+ to C++23, with a modular pluggable rendering backend system (Skia GPU/shader by default), producing a standalone binary with equivalent functionality and pixel-identical output for all non-font-rendering operations.
>
> **Deliverables (Building)**:
> - Standalone `pml` CLI binary (file exec + REPL + --watch mode)
> - `pml-mcp` binary (MCP server for AI agents)
> - Full Google Test suite (389+ tests)
> - Parity verification script
>
> **Estimated Effort**: XL (30+ tasks across 9 waves)
> **Parallel Execution**: YES - 9 waves with 3-6 tasks per wave
> **Critical Path**: CMake scaffolding → Core types → Lexer → Parser → Expander → Evaluator → Builtins → CLI/MCP

---

## Context

### Original Request
Refactor the entire PML (Picture Markup Language) project from Python to C++23, as a new independent git repo. PML is a Lisp-style DSL for code-to-image generation.

### Interview Summary
**Key Decisions**:
- Build system: CMake + vcpkg (with **skia**, giflib, nlohmann-json, googletest)
- Graphics backend: **Skia** (GPU/shader/CPU raster, with giflib for GIF; Skia has built-in image loading)
- AST: `std::variant` + `std::unique_ptr<ListExpr>` for recursive types
- Error handling: `std::expected<Value, Error>` (C++23)
- Testing: Google Test, tests-after-module workflow
- Directory: Deep hierarchy under `src/pml/{core,frontend,evaluator,graphics,backend,sprites,animation,skeleton,module,cli,mcp}/`
- Stdlib: Compile-time embedding (xxd) + runtime loader
- MCP: Rewritten in C++ (nlohmann/json JSON-RPC over stdio)
- Repository: Independent git repo (initially within pml-cpp/ subdir)
- Phasing: Bottom-up (each wave produces testable artifact)

### (Metis Review) Identified Gaps (addressed)

| Gap | Resolution |
|-----|-----------|
| Font rendering differs (platform font stacks) | Accept differences; document in BEHAVIOR_DIFFERENCES.md |
| `id()` identity for animation | Add `uint64_t id` field to GraphicObject + atomic counter |
| SVG path parsing | Use Skia's SkPath + SkParsePath (native, not polygon approximation) |
| Backend hardcoded in render.py | Design pluggable BackendRegistry with compile-time selection |
| Shader support doesn't exist in Python PML | Add as new feature: `(shader "...glsl...")` via SkRuntimeEffect |
| MCP JSON-RPC implementation | Use nlohmann/json for JSON handling |
| 12 PMLError subtypes | Map to `std::variant<ErrorCode, SourceLoc>` |
| Stdlib path resolution | xxd embedding + runtime loader matching Python `Path.stem` |
| Skin binding cross-cutting | Frame hook pipeline: IK solve → skin bind → render |
| REPL without readline | Use linenoise (BSD) or custom std::cin loop |
| --watch file monitoring | Use inotify (Linux); defer Windows to V2 |

---

## Work Objectives

### Core Objective
Port the complete PML interpreter from Python 3.10+ to C++23 with a modular Skia-based rendering backend, achieving full feature parity and pixel-identical PNG output.

### Concrete Deliverables
- `pml-cpp/` — Git repo with complete C++23 source
- `pml` — CLI binary (file exec, REPL, --watch, --json, -o)
- `pml-mcp` — MCP server binary (5 tools over JSON-RPC stdio)
- `pml_tests` — Google Test binary (389+ tests matching Python suite)
- `stdlib/*.pml.h` — Embedded stdlib header files
- `parity_check.py` — Cross-interpreter verification script

### Definition of Done
- [ ] `echo "(+ 1 2)" | ./build/pml` outputs `3`
- [ ] `./build/pml examples/hello.pml -o /tmp/out` produces a PNG matching Python output
- [ ] `./build/pml examples/animation.pml --gif` produces a GIF matching Python output
- [ ] `ctest --test-dir build` passes all tests (exit code 0)
- [ ] `echo 'Content-Length: N' | ./build/pml-mcp` responds with 5 tools
- [ ] `python parity_check.py` reports "ALL PASS: C++ matches Python"

### Must Have
- Backend system is pluggable via `BackendRegistry` — adding a new backend requires only implementing `RenderBackend` ABC + one line of registration
- `NullBackend` always compiled in for testing (records call sequences, fast)
- **SkiaBackend** is the default backend: supports CPU raster, GPU (Vulkan/GL), shaders via SkRuntimeEffect
- `(list-backends)`, `(set-backend! ...)`, `(current-backend)` PML builtins for runtime backend query/switch
- `(shader "...glsl...")` PML builtin compiles SkRuntimeEffect shader, usable as fill/stroke
- All 12 PMLError subtypes properly mapped with SourceLocation info
- Animation system uses explicit GraphicObject.id (not pointer identity)
- GIF palette reduction matches Pillow's ADAPTIVE output
- Stdlib embedded and loadable without external files
- All 5 MCP tools present with identical schemas
- `--watch` mode using inotify on Linux
- Pixel-level PNG comparison tool for render QA

### Must NOT Have (Guardrails)
- No changing existing Python DSL syntax (but shader extension IS new and allowed)
- No fixing existing Python bugs (document in BEHAVIOR_DIFFERENCES.md)
- No SVG/PDF export in V1
- No WebSocket/HTTP MCP transport (stdio only)
- No dynamic .so plugin loading (compile-time registration only)
- No cross-compilation or CI pipeline in V1
- No performance optimization without profiling data
- No over-engineering (no CRTP visitor libraries, no constexpr everything)
- No changes to .pml stdlib file content (byte-identical embedding)

---

## Verification Strategy

> **ZERO HUMAN INTERVENTION** - ALL verification is agent-executed.

### Test Decision
- **Infrastructure**: YES (Google Test)
- **Automated tests**: Tests-after-module
- **Framework**: Google Test (gtest)
- **Workflow**: Each module gets its own test file after implementation

### QA Policy
Every task MUST include agent-executed QA scenarios.
- **CLI/TUI**: Use interactive_bash (tmux) - Run commands, validate output
- **API/Library**: Use Bash (cmake --build) - Build and run GTest suite
- **Rendering**: Use Bash - Run C++ and Python on same .pml, compare PNG hashes
- **MCP**: Use Bash - Send JSON-RPC messages via stdio, validate response

---

## Execution Strategy

### Parallel Execution Waves

```
Wave 1 (Foundation - 6 parallel tasks):
├── Task 1: CMake scaffolding + vcpkg deps
├── Task 2: Core types (Expr, Symbol, Keyword)
├── Task 3: Error types (12 PMLError subtypes)
├── Task 4: Environment (scope chain)
├── Task 5: AffineTransform
└── Task 6: Stdlib embedding (xxd generation)

Wave 2 (Frontend - 3 sequential tasks):
├── Task 7: Lexer
├── Task 8: Parser
└── Task 9: Expander

Wave 3 (Graphics ABC + Registry + Dispatch - 4 parallel tasks):
├── Task 10: GraphicObject + Canvas
├── Task 11: RenderBackend ABC + BackendRegistry + NullBackend
├── Task 12: SkiaBackend (GPU/CPU draw primitives)
├── Task 13: Render dispatch + SVG path parser (Skia)
└── Task 14: Color parsing + helpers (moved here, pure functions)

Wave 4 (GIF + Evaluator - 4 parallel-ish tasks):
├── Task 15: GIF export (giflib integration) [independent, can start early]
├── Task 16: Evaluator (special forms, function application)
└── Task 17: Builtins (arithmetic, IO, list, string, etc.)

Wave 5 (Support Modules - 5 parallel tasks):
├── Task 18: Module system
├── Task 19: Style system
├── Task 20: Palette system
├── Task 21: Easing functions
└── Task 22: Animation + Timeline

Wave 6 (Skeleton/IK - 3 parallel tasks):
├── Task 23: Skeleton types
├── Task 24: FABRIK solver
└── Task 25: CCD solver

Wave 7 (Integration - 3 tasks):
├── Task 26: Skin binding + IK→animation integration
├── Task 27: Spritesheet rendering
├── Task 28: BackendRegistry PML builtins (list-backends, set-backend!, etc.)
└── Task 29: Shader subsystem (SkRuntimeEffect wrapper + (shader "...") builtin)

Wave 8 (CLI + MCP + API - 4 parallel tasks):
├── Task 30: PMLRuntime API facade
├── Task 31: CLI (file exec, REPL, --watch, --json, -o)
├── Task 32: MCP server
└── Task 33: Stdlib runtime loader

Wave 9 (Testing + Parity - 3 parallel tasks):
├── Task 34: Per-module GTest suite
├── Task 35: Parity verification script
└── Task 36: BEHAVIOR_DIFFERENCES.md

Wave FINAL (4 parallel reviews):
├── Task F1: Plan compliance audit (oracle)
├── Task F2: Code quality review (unspecified-high)
├── Task F3: Real manual QA (unspecified-high)
└── Task F4: Scope fidelity check (deep)
```

### Agent Dispatch Summary

- **Wave 1**: T1 → `quick`, T2-T5 → `unspecified-high`, T6 → `quick`
- **Wave 2**: T7-T9 → `unspecified-high`
- **Wave 3**: T10-T14 → `unspecified-high` + `visual-engineering` for T12 (Skia)
- **Wave 4**: T15 → `unspecified-high`, T16-T17 → `unspecified-high`
- **Wave 5**: T18-T22 → `unspecified-high`
- **Wave 6**: T23-T25 → `unspecified-high`
- **Wave 7**: T26-T29 → `unspecified-high` + `visual-engineering` for T29 (shader)
- **Wave 8**: T30-T33 → `unspecified-high`
- **Wave 9**: T34-T36 → `deep`, T35 → `deep`, T36 → `writing`
- **FINAL**: F1 → `oracle`, F2 → `unspecified-high`, F3 → `unspecified-high`, F4 → `deep`

---

---

## Final Verification Wave (MANDATORY — after ALL implementation tasks)

> 4 review agents run in PARALLEL. ALL must APPROVE.

- [ ] F1. **Plan Compliance Audit** — `oracle`
  Read the plan end-to-end. For each must-have: verify implementation exists (read file, curl endpoint, run command). For each must-not-have: search codebase for forbidden patterns — reject with file:line if found. Check evidence files. Compare deliverables against plan.
  Output: `Must Have [N/N] | Must NOT Have [N/N] | Tasks [N/N] | VERDICT: APPROVE/REJECT`

- [ ] F2. **Code Quality Review** — `unspecified-high`
  Build project with `cmake --build build`. Run `ctest`. Review all changed files for: raw pointers (use smart_ptr), C-style casts, using namespace std, magic numbers, empty catch blocks, commented-out code. Check AI slop: excessive comments, over-abstraction, generic names.
  Output: `Build [PASS/FAIL] | Tests [N pass/N fail] | Files [N clean/N issues] | VERDICT`

- [ ] F3. **Real Manual QA** — `unspecified-high`
  Start from clean build. Execute EVERY QA scenario from EVERY task — exact steps, evidence capture. Test cross-task integration: lex→parse→eval→render pipeline, animation→GIF pipeline, skeleton→IK→skin pipeline, CLI→MCP integration. Test edge cases: empty input, invalid syntax, missing files, deep recursion. Save evidence.
  Output: `Scenarios [N/N pass] | Integration [N/N] | Edge Cases [N tested] | VERDICT`

- [ ] F4. **Scope Fidelity Check** — `deep`
  For each task: read "What to do", read actual diff (git log/diff). Verify 1:1 — everything in spec was built (no missing), nothing beyond spec was built (no creep). Check must-not-have compliance. Detect cross-task contamination.
  Output: `Tasks [N/N compliant] | Contamination [CLEAN/N issues] | Unaccounted [CLEAN/N files] | VERDICT`

---

## Commit Strategy

| Task(s) | Commit Message | Scope |
|---------|---------------|-------|
| T1 | `build(cmake): scaffold project with vcpkg deps` | CMakeLists.txt, vcpkg.json, .gitignore |
| T2-T4 | `feat(core): add runtime types, errors, environment` | src/pml/core/ |
| T5 | `feat(graphics): add AffineTransform` | src/pml/graphics/transform.h |
| T6 | `chore(build): add stdlib embedding` | tools/embed_stdlib.py, embedded_stdlib.h |
| T7-T9 | `feat(frontend): add lexer, parser, expander` | src/pml/frontend/ |
| T10-T11 | `feat(graphics): add GraphicObject, Canvas, Backend ABC` | src/pml/graphics/ |
| T12-T14 | `feat(backend): add Skia backend, render dispatch, color` | src/pml/backend/skia/, src/pml/graphics/render.cpp |
| T15 | `feat(eval): add evaluator with special forms` | src/pml/evaluator/evaluator.cpp |
| T16 | `feat(eval): add builtins (arithmetic, IO, list, string)` | src/pml/evaluator/builtins.cpp |
| T17 | `feat(module): add module system` | src/pml/module/ |
| T18-T19 | `feat(sprites): add style and palette systems` | src/pml/sprites/ |
| T20-T21 | `feat(animation): add easing functions and timeline` | src/pml/animation/ |
| T23-T25 | `feat(skeleton): add skeleton types and IK solvers` | src/pml/skeleton/ |
| T26-T27 | `feat(integration): add skin binding, spritesheet` | src/pml/skeleton/, src/pml/graphics/ |
| T28-T29 | `feat(backend-ext): add backend builtins and shader` | src/pml/builtins/, src/pml/backend/skia/ |
| T30 | `feat(api): add PMLRuntime facade` | src/pml/api.cpp |
| T31 | `feat(cli): add CLI, REPL, watch mode` | src/pml/cli/ |
| T32 | `feat(mcp): add MCP server` | src/pml/mcp/ |
| T33 | `feat(module): add stdlib runtime loader` | src/pml/module/ |
| T34 | `test: add Google Test suite (389+ tests)` | tests/ |
| T35 | `test: add parity verification script` | tools/parity_check.py |
| T36 | `docs: add BEHAVIOR_DIFFERENCES.md` | BEHAVIOR_DIFFERENCES.md |

---

## Success Criteria

### Verification Commands
```bash
# Build
cmake -B build && cmake --build build

# Unit tests
ctest --test-dir build --output-on-failure

# CLI smoke test
./build/pml -e "(+ 1 2)"           # → 3

# Render test
./build/pml examples/hello.pml -o /tmp/pml-test/
file /tmp/pml-test/hello.png       # → valid PNG

# GIF test
./build/pml examples/animation.pml -o /tmp/pml-test/
file /tmp/pml-test/animation.gif   # → valid GIF

# JSON output
./build/pml -e "(+ 1 2)" --json    # → {"success": true, ...}

# MCP server
echo -e 'Content-Length: 55\r\n\r\n{"jsonrpc":"2.0","method":"tools/list","id":1}' | ./build/pml-mcp

# Parity check
python tools/parity_check.py --pml-py "uv run pml" --pml-cpp ./build/pml
```

### Final Checklist
- [ ] All 34 implementation tasks complete
- [ ] All 4 final verification tasks pass
- [ ] `ctest` passes (389+ tests, exit 0)
- [ ] `parity_check.py` reports ALL PASS
- [ ] BEHAVIOR_DIFFERENCES.md documents known differences
- [ ] README.md updated with build instructions
- [ ] `./build/pml --help` outputs usage
- [ ] `./build/pml-mcp` responds to JSON-RPC initialize

### Wave 1: Foundation

- [x] 1. **CMake scaffolding + vcpkg dependencies**

  **What to do**:
  - Create `pml-cpp/` directory with CMakeLists.txt structure
  - Set up vcpkg.json with dependencies: skia, giflib, nlohmann-json, googletest
  - Create directory tree: `src/pml/{core,frontend,evaluator,graphics,backend,sprites,animation,skeleton,module,cli,mcp}/`
  - Add CMakeLists.txt per subdirectory
  - Configure C++23 standard (`CMAKE_CXX_STANDARD 23`)
  - Add initial `.gitignore`, `README.md`
  - Verify `cmake -B build && cmake --build build` succeeds with a stub main

  **Must NOT do**:
  - Don't add unnecessary vcpkg ports
  - Don't over-configure CMake (no presets, no toolchain files yet)

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
  - **Skills**: `customize-opencode` (for project config)

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 2-6)
  - **Blocks**: All subsequent tasks
  - **Blocked By**: None

  **Acceptance Criteria**:
  - [ ] `cmake -B build && cmake --build build` produces no errors
  - [ ] `./build/pml` runs and exits with usage message
  - [ ] skia/giflib/nlohmann-json/googletest are findable by CMake
  - [ ] Directory structure matches plan

  **QA Scenarios**:
  ```
  Scenario: Build system works
    Tool: Bash
    Steps:
      1. cd /path/to/pml-cpp && mkdir -p build && cmake -B build
      2. cmake --build build --target pml
    Expected Result: Exit code 0, binary at build/pml
    Evidence: .omo/evidence/task-1-build-success.log
  ```

- [x] 2. **Core runtime types (Expr, Symbol, Keyword, Value, Procedure, BuiltinProcedure, Macro)**

  **What to do**:
  - Implement `src/pml/core/types.h` and `types.cpp`
  - `Symbol` struct (frozen, name string, repr)
  - `Keyword` struct (frozen, name string, repr)
  - `Expr = variant<nullptr_t, int64_t, double, string, bool, Symbol, Keyword, unique_ptr<ListExpr>>`
  - `ListExpr { vector<Expr> elements }`
  - `Procedure` class (params, body, closure_env shared_ptr, optional name)
  - `BuiltinProcedure` class (name, function, accepts_kwargs flag)
  - `Macro` class (name, params, rest_param, body, closure_env, expand method)
  - `Value = variant` covering ALL PML runtime types (nil, int, float, string, bool, Symbol, Keyword, vector<Value>, Procedure, BuiltinProcedure, Macro, Module shared_ptr, GraphicObject, Canvas, AffineTransform, Animation, SkeletonInstance, StyleDescriptor, Palette shared_ptr, Timeline shared_ptr)

  **Must NOT do**:
  - Don't over-abstract; keep variant direct
  - Don't add visitor pattern boilerplate yet

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
  - **Skills**: N/A

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 1, 3-6)
  - **Blocks**: Waves 2-9
  - **Blocked By**: Task 1 (directory structure)

  **Acceptance Criteria**:
  - [ ] All type headers compile with C++23
  - [ ] `Expr` can hold all atom types and recursive lists
  - [ ] `Value` variant includes all runtime types
  - [ ] `Macro::expand()` performs non-hygienic substitution

  **QA Scenarios**:
  ```
  Scenario: Expr value construction
    Tool: Bash (build and run minimal test)
    Steps:
      1. Add temporary test main constructing Expr variants
      2. Build and run
    Expected Result: All Expr constructions compile and print correctly
    Evidence: .omo/evidence/task-2-types-compile.log
  ```

- [x] 3. **Error types (12 PMLError subtypes → Error variant)**

  **What to do**:
  - Implement `src/pml/core/error.h` and `error.cpp`
  - `SourceLocation` struct (line, column, filename)
  - `ErrorCode` enum covering: PMLSyntaxError, PMLTypeError, UnboundVariableError, ArityError, ImportError, CircularImportError, MacroExpansionDepthError, AccessError, ResourceError, IKNoSolutionError, PMLAssertionError, GeneralError
  - `PMLException` struct with error code, source location, message, repair_hint (optional)
  - `Result<T> = expected<T, PMLException>` convenience alias
  - Error formatting matching Python's error output

  **Must NOT do**:
  - Don't use C++ exceptions as primary control flow (use expected)
  - Keep error messages byte-identical with Python where possible

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 1, 2, 4-6)
  - **Blocks**: Evaluator, Parser, Module
  - **Blocked By**: Task 1

  **Acceptance Criteria**:
  - [ ] All 12 error codes present
  - [ ] `Result<T>` alias works with expected
  - [ ] Error messages format with line:col: message

  **QA Scenarios**:
  ```
  Scenario: Error types compile and format
    Tool: Bash
    Steps:
      1. Build with temporary test
    Expected Result: All error codes usable, formatting matches Python style
    Evidence: .omo/evidence/task-3-errors-compile.log
  ```

- [x] 4. **Environment (scope chain)**

  **What to do**:
  - Implement `src/pml/core/environment.h` and `environment.cpp`
  - `Environment` class with parent pointer (shared_ptr), bindings map (string→Value), exports set
  - `lookup(name)` → Result<Value> (searches outward)
  - `define(name, value)` → void (current scope)
  - `set!(name, value)` → Result<void> (mutates outward)
  - `extend(names, values)` → shared_ptr<Environment> (child scope for function calls)
  - `try_lookup(name)` → optional<Value> (no error)
  - Copy semantics: bindings copied for child scopes

  **Must NOT do**:
  - No thread safety yet (single-threaded interpreter)
  - Avoid premature optimization (hash map is fine)

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 1-3, 5, 6)
  - **Blocks**: Evaluator, Module, CLI
  - **Blocked By**: Tasks 2, 3

  **Acceptance Criteria**:
  - [ ] Environment compiles with typed Value
  - [ ] Scoped lookup resolves parent bindings
  - [ ] `extend()` creates child scope without mutating parent
  - [ ] `try_lookup` returns nullopt for unbound symbols

  **QA Scenarios**:
  ```
  Scenario: Environment scoping
    Tool: Bash
    Steps:
      1. Build and run environment test
    Expected Result: Parent/child scoping works correctly
    Evidence: .omo/evidence/task-4-env-scope.log
  ```

- [x] 5. **AffineTransform (2D matrix)**

  **What to do**:
  - Implement `src/pml/graphics/transform.h` and `transform.cpp`
  - Port `AffineTransform` class exactly from `pml/transform.py`
  - Frozen dataclass with 6 float fields (a, b, c, d, e, f)
  - Static factory methods: identity(), translate(), rotate(), scale(), shear()
  - Methods: compose(), inverse(), apply(x,y), apply_points(), to_skmatrix(), is_identity()
  - `to_skmatrix()` returns `SkMatrix` for Skia backend
  - Matrix multiplication order must match Python exactly
  - Float precision must match IEEE 754 double behavior

  **Must NOT do**:
  - No SIMD/optimization yet; keep as direct 6-float struct
  - No BLAS integration

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 1-4, 6)
  - **Blocks**: Graphics backend, Skeleton/IK
  - **Blocked By**: Task 1

  **Acceptance Criteria**:
  - [ ] Matrix multiplication matches Python: translate(5,10).compose(rotate(0.5)) same result
  - [ ] Inverse() handles identity, translation, rotation, scale
  - [ ] apply() transforms (x,y) correctly
  - [ ] Singular matrix raises proper error

  **QA Scenarios**:
  ```
  Scenario: Transform math matches Python
    Tool: Bash
    Steps:
      1. Build transform test
      2. Run test comparing C++ results with Python reference
    Expected Result: All matrix operations produce identical values
    Evidence: .omo/evidence/task-5-transform-math.log
  ```

- [x] 6. **Stdlib embedding (xxd generation)**

  **What to do**:
  - Create a Python script `tools/embed_stdlib.py` (or CMake custom command)
  - Run `xxd -i` on every file in `stdlib/` (math.pml, color.pml, easing.pml, shapes.pml, sprites/*)
  - Generate `src/pml/module/embedded_stdlib.h` with:
    - Byte arrays for each file
    - File path → byte array mapping
    - Total file count + name lookup table
  - Add CMake custom command to auto-regenerate when stdlib files change
  - Verify generated header compiles

  **Must NOT do**:
  - Don't modify or reformat .pml files (embed exactly)
  - Don't use C++23 std::embed (not yet universally supported)

  **Recommended Agent Profile**:
  - **Category**: `quick`

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 1-5)
  - **Blocks**: Task 31 (Stdlib runtime loader)
  - **Blocked By**: Task 1

  **Acceptance Criteria**:
  - [ ] `embedded_stdlib.h` compiles without errors
  - [ ] Contains all stdlib files
  - [ ] Re-generates when stdlib files change
  - [ ] File contents are byte-identical to originals

  **QA Scenarios**:
  ```
  Scenario: Stdlib embedding
    Tool: Bash
    Steps:
      1. Run embed script
      2. Compile generated header
      3. Verify md5sum of embedded content matches originals
    Expected Result: Header compiles, content matches
    Evidence: .omo/evidence/task-6-embed-stdlib.log
  ```

---

### Wave 2: Frontend Pipeline

- [x] 7. **Lexer (source → Token stream)**

  **What to do**:
  - Implement `src/pml/frontend/lexer.h` and `lexer.cpp`
  - Port `pml/lexer.py` exactly: Lexer class, tokenize() method
  - TokenType enum (LPAREN, RPAREN, INTEGER, FLOAT, STRING, BOOLEAN, SYMBOL, KEYWORD, QUOTE, QUASIQUOTE, UNQUOTE, UNQUOTE_SPLICE, EOF)
  - Token struct (type, value string, line, column)
  - String escape sequences (\n, \t, \\, \")
  - Number parsing (integer, float, leading +/-, multiple dots rejection)
  - Boolean parsing (#t / #f)
  - Keyword parsing (:keyword)
  - Comment handling (; line comments)
  - Quote/backquote/unquote sugar (', `, ,@)
  - SourceLocation tracking through character stream

  **Must NOT do**:
  - No regex-based lexing; direct char-by-char (matching Python)
  - No Unicode normalization (PML assumes ASCII identifiers)

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
  - **Skills**: N/A

  **Parallelization**:
  - **Can Run In Parallel**: NO (sequential within Wave 2)
  - **Parallel Group**: Wave 2 lead task
  - **Blocks**: Task 8 (Parser)
  - **Blocked By**: Tasks 2, 3 (types + errors)

  **Acceptance Criteria**:
  - [ ] `tokenize("(+ 1 2)")` produces 5 tokens: LPAREN, SYMBOL, INT, INT, RPAREN, EOF
  - [ ] String escapes (\n → newline, etc.) work correctly
  - [ ] Comments are correctly skipped
  - [ ] All quote/backquote/unquote sugar tokens produced
  - [ ] Error on unterminated string
  - [ ] Token positions (line, col) are accurate

  **QA Scenarios**:
  ```
  Scenario: Basic tokenization
    Tool: Bash
    Steps:
      1. Build and run lexer tests
    Expected Result: All token types produced correctly
    Evidence: .omo/evidence/task-7-lexer-basic.log

  Scenario: String escapes
    Tool: Bash
    Steps:
      1. Lex "\"hello\\nworld\"" 
    Expected Result: Single STRING token with newline embedded
    Evidence: .omo/evidence/task-7-lexer-escapes.log
  ```

- [x] 8. **Parser (Token stream → AST)**

  **What to do**:
  - Implement `src/pml/frontend/parser.h` and `parser.cpp`
  - Port `pml/parser.py` exactly
  - Parser class consuming Token list, producing `vector<Expr>`
  - `parse()` entry point → `vector<Expr>` (top-level expressions)
  - Expression parsing: lists `(...)`, atoms, quote sugar
  - Recursive descent: `_parse_expr()`, `_parse_list()`, `_parse_atom()`
  - Error on unmatched `(` with line/column info
  - Must produce AST identical to Python parser for same input

  **Must NOT do**:
  - No PEG/parser generator; keep recursive descent matching Python
  - No AST optimization (keep structure matching Python output)

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: NO (sequential within Wave 2)
  - **Parallel Group**: Wave 2 (after Task 7)
  - **Blocks**: Task 9 (Expander)
  - **Blocked By**: Task 7 (Token types)

  **Acceptance Criteria**:
  - [ ] `parse("(+ 1 2)")` produces `[List(Symbol(+), Int(1), Int(2))]`
  - [ ] Nested lists: `(a (b c))` → `[List(Sym a, List(Sym b, Sym c))]`
  - [ ] Quote sugar: `'x` → `[List(Sym quote, Sym x)]`
  - [ ] Error on unmatched paren with correct line/col
  - [ ] Empty input → empty list

  **QA Scenarios**:
  ```
  Scenario: Basic parsing
    Tool: Bash
    Steps:
      1. Build and run parser tests
    Expected Result: AST structure matches Python parser output
    Evidence: .omo/evidence/task-8-parser-basic.log
  ```

- [x] 9. **Expander (macro expansion pass)**

  **What to do**:
  - Implement `src/pml/frontend/expander.h` and `expander.cpp`
  - Port `pml/expander.py` exactly
  - Expander class with Environment reference
  - `expand(expr, depth=0)` → Expr (recursive macro expansion)
  - `expand_all(ast)` → vector<Expr> (expand all top-level)
  - Special form protection: don't expand inside `quote`
  - Lambda/defmacro: expand body only, not parameter list
  - Define: expand body for `(define (name params) body...)`
  - Macro expansion depth limit (default 256)
  - Non-hygienic expansion (matching Python behavior)

  **Must NOT do**:
  - No hygienic macros (must match Python's non-hygienic behavior)
  - No `syntax-rules` style matching (not in PML)

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: NO (sequential within Wave 2)
  - **Parallel Group**: Wave 2 (after Task 8)
  - **Blocks**: Evaluator
  - **Blocked By**: Task 8 (AST types)

  **Acceptance Criteria**:
  - [ ] `expand([[Sym('if'), [Sym('macro?'), Sym('x')], Sym('yes'), Sym('no')]])` doesn't expand inside quote
  - [ ] Macro definition (defmacro) expands body but not params
  - [ ] Depth limit exceeded raises proper error

  **QA Scenarios**:
  ```
  Scenario: Basic expansion
    Tool: Bash
    Steps:
      1. Build and run expander tests
    Expected Result: Macro expansion matches Python behavior
    Evidence: .omo/evidence/task-9-expander-basic.log
  ```

---

### Wave 3: Graphics ABC + Registry + Dispatch

- [x] 10. **GraphicObject + Canvas**

  **What to do**:
  - Implement `src/pml/graphics/objects.h/.cpp` and `canvas.h/.cpp`
  - `GraphicObject` struct (immutable/frozen):
    - shape_type: string
    - params: map<string, Value>
    - fill: optional<string>
    - stroke: optional<string>
    - stroke_width: float
    - transform: AffineTransform
    - children: vector<GraphicObject>
    - metadata: map<string, Value>
    - id: uint64_t (atomic counter for animation identity)
    - with_transform(), with_fill(), with_stroke(), with_param() — return new objects
  - `Canvas` struct:
    - width, height: int
    - bg_color: string
    - objects: vector<GraphicObject>
    - anchor: string
    - padding: int
    - is_sprite: bool
    - add(obj) method
  - Global current canvas singleton pattern (matching Python)
  - `_canvas(w, h, **kwargs)` and `_sprite_canvas(w, h, **kwargs)` factory functions

  **Must NOT do**:
  - Don't add rendering logic to GraphicObject (separate concern)
  - id assignment must use atomic<uint64_t> for thread safety

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 3 (with Tasks 11-14)
  - **Blocks**: Render dispatch, Sprites, Animation, Skeleton
  - **Blocked By**: Tasks 2, 5 (types + transform)

  **Acceptance Criteria**:
  - [ ] GraphicObject immutable: new object returned for each mutation
  - [ ] Canvas.add() appends to objects vector
  - [ ] id field auto-assigned, unique, monotonically increasing
  - [ ] Global canvas singleton accessible

  **QA Scenarios**:
  ```
  Scenario: GraphicObject creation
    Tool: Bash
    Steps:
      1. Build and run graphics object tests
    Expected Result: Objects created, modified immutably, IDs unique
    Evidence: .omo/evidence/task-10-graphics-basic.log
  ```

- [x] 11. **RenderBackend ABC + BackendRegistry + NullBackend**

  **What to do**:
  - Implement `src/pml/backend/backend.h`, `registry.h/.cpp`, `capabilities.h`, `null_backend.cpp`
  - **`capabilities.h`**: `BackendCap` enum (RasterCPU, GPUAccel, Shaders, VectorOutput, AnimationGIF, FontRendering, LoadPNG, etc.) + `BackendInfo{name, description, capabilities}`
  - **`backend.h`**: `RenderBackend` ABC with virtual methods:
    - `info() → BackendInfo`
    - `create_surface(w, h, bg) → unique_ptr<Surface>`
    - `draw(surface, obj)` → void
    - `save_image(surface, path, format)` → void
    - `save_animation(frames, path, format, fps)` → void (optional)
    - `compile_shader(glsl) → unique_ptr<Shader>` (optional)
  - **`registry.h/.cpp`**: `BackendRegistry` singleton:
    - `add(name, factory, info)` — register a backend
    - `create(name) → unique_ptr<RenderBackend>` — create by name
    - `create_best(required_cap) → unique_ptr<RenderBackend>` — auto-select best
    - `available() → vector<BackendInfo>` — list registered backends
    - `set_active(name)` / `active() → RenderBackend&` — active backend management
  - **`null_backend.cpp`**: `NullBackend : RenderBackend`:
    - Does no actual rendering, records all calls
    - Always compiled in (no GPU/library dependency)
    - Perfect for CI testing
  - Static registration: each backend .cpp uses `[[maybe_unused]] static bool` to self-register

  **Must NOT do**:
  - No .so plugin loading (compile-time registration only)

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 3 (with Tasks 10, 12, 13)
  - **Blocks**: Wave 4 (SkiaBackend, Render dispatch)
  - **Blocked By**: Tasks 10 (GraphicObject)

  **Acceptance Criteria**:
  - [ ] BackendRegistry singleton compiles and is accessible
  - [ ] `add()` + `create("null")` returns a NullBackend
  - [ ] `available()` returns ["null"] when only NullBackend registered
  - [ ] `set_active()` + `active()` returns correct backend
  - [ ] NullBackend records draw() calls and can replay them
  - [ ] RenderBackend ABC can be subclassed with all methods

  **QA Scenarios**:
  ```
  Scenario: BackendRegistry create
    Tool: Bash
    Steps:
      1. Register NullBackend
      2. create("null") → non-null pointer
    Expected Result: NullBackend created successfully
    Evidence: .omo/evidence/task-11-registry-create.log

  Scenario: NullBackend records calls
    Tool: Bash
    Steps:
      1. Create NullBackend, call draw() 3 times
      2. Check call_log size == 3
    Expected Result: Call recording works
    Evidence: .omo/evidence/task-11-null-recording.log
  ```

- [~] 12. **SkiaBackend (GPU/CPU draw primitives)**
  > **BLOCKED**: Skia third-party dependencies (`third_party/externals/`) require `googlesource.com` downloads which are not accessible from this build environment. Cairo backend used as working alternative.

  **What to do**:
  - Implement `src/pml/backend/skia/skia_backend.h/.cpp`
  - Subclass RenderBackend using Skia
  - `create_surface()` → `SkSurfaces::Raster()` or `GrContext + SkSurface::MakeRenderTarget()`
  - `draw()` dispatch for each shape_type using Skia API:
    - circle → `SkCanvas::drawCircle()`
    - rect → `SkCanvas::drawRect()` (axis-aligned) or `drawPath()` (rotated)
    - ellipse → `SkCanvas::drawOval()`
    - line → `SkCanvas::drawLine()`
    - polygon → `SkPath::addPoly()` + `drawPath()`
    - path → `SkParsePath::ToSkPath()` + `drawPath()` (native SVG path support)
    - text → `SkFont` + `SkCanvas::drawString()` with platform font
    - image → `SkImage::MakeFromEncoded()` (Skia built-in: PNG/JPEG/WEBP/GIF)
    - group → `SkCanvas::save()`/`restore()` with transform composition
  - `save_image()` → `SkPngEncoder::Encode()` / `SkJpegEncoder::Encode()` / `SkWebpEncoder::Encode()`
  - GPU mode: `GrDirectContext::MakeGL()` / `MakeVulkan()`
  - CPU mode: `SkSurfaces::Raster()` (always works, no GPU needed)
  - Auto-registers to BackendRegistry via static init
  - AffineTransform integration: `SkCanvas::setMatrix(SkMatrix)`

  **Must NOT do**:
  - No shader handling here (separate task 30)
  - No GIF handling here (separate task 16)

  **Recommended Agent Profile**:
  - **Category**: `visual-engineering`
  - **Skills**: N/A

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 4 (with Tasks 14, 16)
  - **Blocks**: Render dispatch, Sprites, Shader subsystem
  - **Blocked By**: Tasks 11 (BackendRegistry + ABC)

  **Acceptance Criteria**:
  - [ ] Skia renders basic shapes: circle, rect, ellipse, line, polygon
  - [ ] SVG path rendering via SkParsePath
  - [ ] Image loading via SkImage::MakeFromEncoded()
  - [ ] AffineTransform applied before drawing (SkMatrix)
  - [ ] Both CPU raster and GPU surface modes work
  - [ ] Self-registers as "skia" in BackendRegistry

  **QA Scenarios**:
  ```
  Scenario: Skia renders a circle
    Tool: Bash
    Steps:
      1. Create GraphicObject(circle, {x:50,y:50,r:30}, fill="#ff0000")
      2. Call SkiaBackend::draw on SkSurface
      3. Save to PNG via SkPngEncoder
    Expected Result: PNG of red circle at (50,50) radius 30
    Evidence: .omo/evidence/task-12-skia-circle.png
  ```

- [x] 13. **Render dispatch + SVG path parser (Cairo-compatible)**

  **What to do**:
  - Implement `src/pml/graphics/render.h/.cpp`
  - `_render(filename, **kwargs)` — render current canvas to file
    - Determine format from extension or kwargs
    - Query `BackendRegistry::active()` for the backend
    - Create surface via `backend.create_surface()`
    - Dispatch each canvas object to `backend.draw()`
    - Apply sprite auto-centering for sprite canvases
    - Save image via `backend.save_image()`
  - `_render_set(name, **kwargs)` — render at multiple scales
  - `_render_spritesheet(*args, **kwargs)` — render sprites to grid
  - SVG path parser uses **Skia's SkParsePath** (no custom parser needed):
    - `SkParsePath::ToSVGString(path)` / `SkParsePath::FromSVGString(string)`
    - Handle M, L, C, Q, A, Z commands (Skia natively supports full SVG spec)
  - `register_render(env)` — bind render functions to PML environment

  **Must NOT do**:
  - No GIF handling here (separate task 16)
  - Must query BackendRegistry, not hardcode backend

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 3 (with Tasks 10-12)
  - **Blocks**: CLI, Animation→GIF
  - **Blocked By**: Tasks 11, 12 (BackendRegistry + SkiaBackend)

  **Acceptance Criteria**:
  - [ ] `_render("out.png")` produces valid PNG via active backend
  - [ ] `_render_set("out", scales=[1,2,4])` produces @1x, @2x, @4x variants
  - [ ] `_render_spritesheet` produces correctly gridded spritesheet + metadata JSON
  - [ ] SVG path parsing via SkParsePath (M, L, C, Q, A, Z)

  **QA Scenarios**:
  ```
  Scenario: Render dispatch via active backend
    Tool: Bash
    Steps:
      1. Set active backend to "null"
      2. Create canvas, add rect, call _render
      3. Check NullBackend call_log for draw() recording
    Expected Result: Render dispatches to active backend correctly
    Evidence: .omo/evidence/task-13-render-dispatch.log
  ```

- [x] 14. **Color parsing + helpers**

  **What to do**:
  - Implement color parsing utilities in `src/pml/backend/color_helpers.h/.cpp`
  - Named colors: "red", "blue", "green", "white", "black", "transparent", etc.
  - Hex parsing: "#RGB", "#RRGGBB", "#RRGGBBAA"
  - `parse_color(string)` → `optional<SkColor>` (uint32_t ARGB)
  - Skia-compatible: SkColor is premultiplied ARGB, direct match
  - Match Python `parse_color()` output exactly
  - `apply_color(canvas, color_string)` — set paint color from string

  **Must NOT do**:
  - No CSS color parsing beyond what Python supports

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 4 (with Tasks 12, 16)
  - **Blocks**: SkiaBackend rendering
  - **Blocked By**: None (pure functions)

  **Acceptance Criteria**:
  - [ ] All named colors parse correctly
  - [ ] #RGB, #RRGGBB, #RRGGBBAA all work
  - [ ] "transparent" → 0 (SkColor with alpha=0)
  - [ ] Unknown color → nullopt

  **QA Scenarios**:
  ```
  Scenario: Color parsing
    Tool: Bash
    Steps:
      1. Build and run color parsing tests
    Expected Result: All color formats parsed correctly
    Evidence: .omo/evidence/task-14-color-parsing.log
  ```

---

### Wave 4: GIF + Evaluator + Builtins

- [x] 15. **GIF export (giflib integration) [independent, can start early]**

  **What to do**:
  - Integrate giflib for animated GIF output
  - `save_gif(frames, path, fps)` → void
    - Convert each frame (Skia surface) to GIF-compatible format
    - Read pixels back from SkSurface → RGBA buffer
    - RGBA → composite onto white background (matching Python Pillow behavior)
    - Reduce colors to 256 (match Pillow's ADAPTIVE palette behavior)
    - Write GIF frames with proper disposal mode (2 = restore to background color)
  - GIF LZW encoding via giflib
  - Invoked by `SkiaBackend::save_animation()` when format is GIF
  - Must produce GIFs visually identical to Python Pillow output

  **Must NOT do**:
  - No WebP/APNG/MP4 export
  - Does NOT depend on Skia being the active backend (pure CPU operation)

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: YES (pure CPU work, no GPU dependency)
  - **Parallel Group**: Wave 4 (with Tasks 16, 17)
  - **Blocks**: MCP server rendering tools
  - **Blocked By**: None (independent utility library)

  **Acceptance Criteria**:
  - [ ] GIF file produced with correct frame count
  - [ ] GIF matches Python Pillow output frame-by-frame (pixel comparison)
  - [ ] Proper disposal mode applied
  - [ ] RGBA→white background compositing matches Python exactly

  **QA Scenarios**:
  ```
  Scenario: GIF export
    Tool: Bash
    Steps:
      1. Create animation with 10 frames
      2. Export as GIF
      3. Verify frame count and dimensions
      4. Compare frames with Python Pillow output
    Expected Result: GIF produced, frames match Pillow
    Evidence: .omo/evidence/task-15-gif-export.gif
  ```

- [x] 16. **Evaluator (special forms, function application)**

  **What to do**:
  - Implement `src/pml/evaluator/evaluator.h/.cpp`
  - Port `pml/evaluator.py` exactly
  - `evaluate(Expr, Environment)` → Result<Value>
  - Self-evaluating atoms: int, float, bool, string, nil
  - Symbol lookup in environment
  - Module access: `prefix/symbol` → module.get(symbol)
  - Special forms map: `if`, `define`, `lambda`, `quote`, `quasiquote`, `set!`, `begin`, `cond`, `and`, `or`, `let`, `let*`, `letrec`, `define-macro`, `defmacro`, `import`, `provide`, `macroexpand`, `assert`, `gensym`
  - Function application: evaluate args, apply function
  - Macro expansion during evaluation (macro value check)
  - Argument evaluation: `evaluate_arguments()` separating positional and keyword args
  - `apply_function()` dispatching on Procedure, BuiltinProcedure
  - `_expand_macro()` with depth tracking
  - Capture `__source_file__` in module evaluation
  - All special forms must match Python semantics exactly

  **Must NOT do**:
  - No tail-call optimization (Python doesn't either)
  - No JIT or compilation

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: NO (depends on frontend)
  - **Parallel Group**: Wave 4 (with Task 17)
  - **Blocks**: Waves 5-9
  - **Blocked By**: Tasks 4, 7, 8, 9 (env + frontend pipeline)

  **Acceptance Criteria**:
  - [ ] `evaluate(Symbol("+"), env)` → builtin + function
  - [ ] `evaluate(["+", 1, 2], env)` → 3
  - [ ] `evaluate([lambda, [x], [*, x, 2]], env)` → Procedure
  - [ ] `(if #t "yes" "no")` → "yes"
  - [ ] `(define x 5)` binds x in environment
  - [ ] `(quote (1 2 3))` → list without evaluation
  - [ ] `(import "math.pml" as math)` loads module
  - [ ] `(provide x y)` exports symbols
  - [ ] Macro expansion during evaluation works

  **QA Scenarios**:
  ```
  Scenario: Basic evaluation
    Tool: Bash
    Steps:
      1. `evaluate(["+", 1, 2], env)`
    Expected Result: 3
    Evidence: .omo/evidence/task-16-eval-basic.log

  Scenario: Special forms
    Tool: Bash
    Steps:
      1. Test if, define, lambda, quote, begin
    Expected Result: All produce correct results matching Python
    Evidence: .omo/evidence/task-16-eval-special.log
  ```

- [x] 17. **Builtins (arithmetic, IO, list, string, type predicates)**

  **What to do**:
  - Implement `src/pml/evaluator/builtins.h/.cpp`
  - Port all modules from `pml/builtins/`:
    - Arithmetic: +, -, *, /, modulo, abs, min, max, expt, sqrt, floor, ceiling, round
    - Comparison: =, <, >, <=, >=, eq?, equal?, string=?, string<?
    - IO: display, newline, read, read-line, string->symbol, symbol->string
    - List: car, cdr, cons, list, append, length, reverse, map, filter, reduce, list-ref
    - String: string, string-append, string-length, string-ref, substring, string-split
    - Type predicates: number?, integer?, float?, string?, boolean?, symbol?, keyword?, list?, procedure?, null?, pair?
    - Transform builtins: translate, rotate, scale, shear, compose, identity-matrix, matrix-inverse, matrix-apply
  - Each builtin as a `BuiltinProcedure` registered via `register_builtins(env)`
  - Arity checking matching Python builtins

  **Must NOT do**:
  - Don't add Python-specific builtins that don't exist in PML

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Task 16)
  - **Parallel Group**: Wave 4 (with Task 16)
  - **Blocks**: Module system, CLI
  - **Blocked By**: Tasks 2, 3, 4 (types + env)

  **Acceptance Criteria**:
  - [ ] `(+ 1 2 3)` → 6
  - [ ] `(car (list 1 2 3))` → 1
  - [ ] `(string-append "a" "b")` → "ab"
  - [ ] `(number? 42)` → #t
  - [ ] `(translate 10 20)` → AffineTransform
  - [ ] All error cases properly reported with line/col

  **QA Scenarios**:
  ```
  Scenario: Arithmetic builtins
    Tool: Bash
    Steps:
      1. Test all arithmetic ops
    Expected Result: Results match Python PML
    Evidence: .omo/evidence/task-16-builtins-arithmetic.log

  Scenario: List builtins
    Tool: Bash
    Steps:
      1. Test car, cdr, cons, map, filter
    Expected Result: List operations match Python
    Evidence: .omo/evidence/task-16-builtins-list.log
  ```

---

### Wave 5: Support Modules

- [x] 18. **Module system**

  **What to do**:
  - Implement `src/pml/module/module.h/.cpp`
  - Port `pml/module_loader.py` exactly
  - `Module` class: name, env (shared_ptr<Environment>), exports set
  - `get(symbol)` → Result<Value> (checks exports)
  - `ModuleLoader` class: global env reference, cache (path→Module)
  - `load(path, from_file)` → Result<shared_ptr<Module>>
    - Resolve relative paths (from_file directory)
    - Try path as-is, then path + ".pml"
    - Check cache before loading
    - Load: read source → tokenize → parse → expand → evaluate (with `__source_file__` set)
    - Cache the module
    - Circular dependency detection
  - `_eval_import`: `(import "path" as prefix)` — load and bind
  - `_eval_provide`: `(provide sym ...)` — add to exports

  **Must NOT do**:
  - No filesystem watching (module cache is static)

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 5 (with Tasks 19-22)
  - **Blocks**: Wave 8 (Stdlib loader)
  - **Blocked By**: Tasks 16 (evaluator)

  **Acceptance Criteria**:
  - [ ] `(import "math.pml" as m)` loads and binds module
  - [ ] `m/sin` accesses exported symbol
  - [ ] `(provide x y)` works in module files
  - [ ] Circular import detection raises proper error
  - [ ] Module cache prevents re-loading
  - [ ] Relative paths resolved correctly

  **QA Scenarios**:
  ```
  Scenario: Module import
    Tool: Bash
    Steps:
      1. Create test .pml module
      2. Import and use exported symbols
    Expected Result: Module loads, exports accessible
    Evidence: .omo/evidence/task-18-module-basic.log
  ```

- [x] 19. **Style system**

  **What to do**:
  - Implement `src/pml/sprites/style.h/.cpp`
  - Port `pml/sprites/style.py` exactly
  - `StyleDescriptor` struct: outline_width, outline_color, outline_style, shading, shadow, highlight, pixel_size, anti_alias, corner_radius
  - `to_kwargs()` → flat dict for graphic primitives
  - `merge(overrides)` → new StyleDescriptor with overrides
  - Predefined styles: "cel", "pixel", "flat"
  - Global style registry
  - `get_style(name)` → StyleDescriptor
  - `resolve_style(Symbol|str|StyleDescriptor)` → StyleDescriptor
  - `register_style(env)` — `define-style`, `use-style` builtins

  **Must NOT do**:
  - No rendering logic; style is data only

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 5 (with Tasks 17, 19-21)
  - **Blocks**: Sprites rendering
  - **Blocked By**: Tasks 10 (GraphicObject)

  **Acceptance Criteria**:
  - [ ] StyleDescriptor can be created, merged, serialized to kwargs
  - [ ] Predefined styles ("cel", "pixel", "flat") available
  - [ ] `define-style` and `use-style` builtins registered

  **QA Scenarios**:
  ```
  Scenario: Style system
    Tool: Bash
    Steps:
      1. Create and merge StyleDescriptors
    Expected Result: Merging correctly overrides fields
    Evidence: .omo/evidence/task-18-style-basic.log
  ```

- [x] 20. **Palette system**

  **What to do**:
  - Implement `src/pml/sprites/palette.h/.cpp`
  - Port `pml/sprites/palette.py` exactly
  - `Palette` struct: name, colors map (string→string)
  - `get(key)` → string color (fallback #808080)
  - Predefined palettes: "dark-hero", "warm-skin"
  - Active palette singleton
  - `register_palette(env)` — `define-palette`, `palette-ref` builtins

  **Must NOT do**:
  - Keep it simple; no palette interpolation

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 5 (with Tasks 17, 18, 20, 21)
  - **Blocks**: Character sprite assembly
  - **Blocked By**: None

  **Acceptance Criteria**:
  - [ ] Palette stores named colors
  - [ ] Predefined palettes available
  - [ ] `palette-ref` looks up active palette
  - [ ] Unknown key returns fallback

  **QA Scenarios**:
  ```
  Scenario: Palette system
    Tool: Bash
    Steps:
      1. Create palette with colors
      2. Look up colors by key
    Expected Result: Correct colors returned
    Evidence: .omo/evidence/task-19-palette-basic.log
  ```

- [x] 21. **Easing functions**

  **What to do**:
  - Implement `src/pml/animation/easing.h/.cpp`
  - Port `pml/animation/easing.py` exactly
  - All 12 easing functions: linear, quad-in, quad-out, quad-in-out, cubic-in, cubic-out, cubic-in-out, sin-in, sin-out, sin-in-out, bounce, elastic
  - Each function maps t ∈ [0,1] → [0,1]
  - `get_easing(name)` → function pointer/std::function
  - `list_easings()` → available names
  - Float precision must match Python exactly (IEEE 754 double)

  **Must NOT do**:
  - No extra easing functions beyond Python version
  - No constexpr optimization (match Python runtime behavior)

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 5 (with Tasks 17-19, 21)
  - **Blocks**: Animation timeline
  - **Blocked By**: None

  **Acceptance Criteria**:
  - [ ] All 12 functions produce values matching Python at key points (t=0, 0.25, 0.5, 0.75, 1.0)
  - [ ] `get_easing("linear")` returns linear function
  - [ ] `get_easing("unknown")` falls back to linear

  **QA Scenarios**:
  ```
  Scenario: Easing functions
    Tool: Bash
    Steps:
      1. Call each easing function at sample points
      2. Compare with Python reference values
    Expected Result: Values match within floating-point tolerance
    Evidence: .omo/evidence/task-20-easing-results.log
  ```

- [x] 22. **Animation + Timeline**

  **What to do**:
  - Implement `src/pml/animation/timeline.h/.cpp`
  - Port `pml/animation/__init__.py` and `timeline.py` exactly
  - `Animation` struct:
    - target_id (uint64_t — matching GraphicObject.id)
    - property_name (string)
    - from_value / to_value
    - duration (float)
    - easing (function pointer from easing system)
    - get_value_at(t) → vector<(target_id, property, value)>
  - `Timeline` class:
    - animations list
    - frame_hooks callback list
    - state: idle | playing | paused | finished
    - current_time
    - add(anim), play(), stop(), pause(), seek(t)
    - get_total_duration() → float
    - evaluate_at(t) → modifications list
    - render_frames() → sequence of rendered images
  - `_animate()` PML builtin — create and register animation
  - `_play()`, `_stop()`, `_pause()`, `_seek()` PML builtins
  - Global timeline singleton
  - Frame hooks for IK integration
  - `_apply_modifications()` — apply animation modifications to GraphicObject

  **Must NOT do**:
  - No GIF export here (separate task 26)
  - Must use GraphicObject.id, not pointer identity

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 5 (with Tasks 17-20)
  - **Blocks**: GIF export, Skin binding
  - **Blocked By**: Tasks 10, 20 (GraphicObject + Easing)

  **Acceptance Criteria**:
  - [ ] Animation created and registered on timeline
  - [ ] evaluate_at(t) produces correct interpolated values
  - [ ] Frame hooks called before each frame render
  - [ ] get_total_duration() computed correctly
  - [ ] Timeline state machine works (play → pause → resume)

  **QA Scenarios**:
  ```
  Scenario: Timeline evaluation
    Tool: Bash
    Steps:
      1. Create animation: animate ball 'x 0 100 2.0
      2. evaluate_at(t=1.0) → ball.x = 50
    Expected Result: Correct interpolation at midpoint
    Evidence: .omo/evidence/task-21-timeline-eval.log
  ```

---

### Wave 6: Skeleton / IK

- [x] 23. **Skeleton types (Joint, SkeletonTemplate, SkeletonInstance)**

  **What to do**:
  - Implement `src/pml/skeleton/skeleton.h/.cpp`
  - Port `pml/skeleton/skeleton.py` exactly
  - `Joint` struct (frozen): name, pos (dx,dy tuple), length, angle, min_angle, max_angle (optional)
  - `SkeletonTemplate` struct (frozen): name, root_params, joints vector
    - `joint_index(name)` → int index
  - `SkeletonInstance` class:
    - template reference
    - root_x, root_y
    - angles vector (mutable)
    - `chain_positions(end_effector)` → vector of (x,y) positions
    - `bone_lengths(end_effector)` → vector of lengths
    - `set_angle(index, value)` with clamping to min/max
    - `get_joint_world_pos(name)` → (x,y)
    - `forward_kinematics()` → compute all joint positions from angles
  - `_defskeleton()` PML builtin — define a skeleton template
  - `_instantiate_skeleton()` PML builtin — create instance
  - `_joint_position()` PML builtin — query joint position

  **Must NOT do**:
  - No IK solving in this task (separate tasks 23, 24)

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 6 (with Tasks 23, 24)
  - **Blocks**: IK solvers, Skin binding
  - **Blocked By**: Task 5 (AffineTransform)

  **Acceptance Criteria**:
  - [ ] Joint positions computed via forward kinematics
  - [ ] Angle clamping respects min/max constraints
  - [ ] chain_positions() returns correct world-space positions
  - [ ] `defskeleton` and `instantiate-skeleton` builtins work

  **QA Scenarios**:
  ```
  Scenario: Forward kinematics
    Tool: Bash
    Steps:
      1. Create skeleton: shoulder→elbow→wrist
      2. Compute forward kinematics
      3. Verify joint positions match expected
    Expected Result: Joint positions computed correctly
    Evidence: .omo/evidence/task-22-skeleton-fk.log
  ```

- [x] 24. **FABRIK IK solver**

  **What to do**:
  - Implement `src/pml/skeleton/ik_fabrik.h/.cpp`
  - Port `pml/skeleton/ik_fabrik.py` exactly
  - `solve_fabrik(skeleton, end_effector, target_x, target_y, max_iterations=10, tolerance=0.01)` → bool
  - FABRIK algorithm:
    - Backward pass: end effector → target, adjust chain backward
    - Forward pass: fix root, adjust chain forward
    - Convert final positions to joint angles
  - Handle edge cases:
    - Target unreachable (stretch toward target → return false)
    - Single-joint chain (always converged)
    - Zero-length bone segments
  - `_positions_to_angles()` — world positions → clamped joint angles
  - `_normalize()` — vector normalization

  **Must NOT do**:
  - Must match Python FABRIK output exactly (same convergence behavior)

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 6 (with Tasks 22, 24)
  - **Blocks**: IK integration
  - **Blocked By**: Task 22 (Skeleton types)

  **Acceptance Criteria**:
  - [ ] Solver converges within tolerance for reachable targets
  - [ ] Returns false for unreachable targets (stretches toward)
  - [ ] Joint angles clamped to constraints
  - [ ] Results match Python solver on test cases

  **QA Scenarios**:
  ```
  Scenario: FABRIK solve
    Tool: Bash
    Steps:
      1. Create 3-joint arm skeleton
      2. Solve IK to reach known target
      3. Verify end effector is within tolerance of target
    Expected Result: End effector reaches target
    Evidence: .omo/evidence/task-23-fabrik-solve.log
  ```

- [x] 25. **CCD IK solver**

  **What to do**:
  - Implement `src/pml/skeleton/ik_ccd.h/.cpp`
  - Port `pml/skeleton/ik_ccd.py` exactly
  - `solve_ccd(skeleton, end_effector, target_x, target_y, max_iterations=20, tolerance=0.01)` → bool
  - CCD algorithm: iterate joints from end to root, rotate each to minimize angle error
  - Handle edge cases same as FABRIK
  - `register_ik(env)` — register `ik-solve` builtin dispatching to FABRIK or CCD by method name

  **Must NOT do**:
  - Must match Python CCD output exactly

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 6 (with Tasks 22, 23)
  - **Blocks**: IK integration
  - **Blocked By**: Task 22 (Skeleton types)

  **Acceptance Criteria**:
  - [ ] Solver converges within tolerance
  - [ ] Results match Python CCD solver on test cases
  - [ ] `ik-solve` builtin dispatches correctly to FABRIK or CCD

  **QA Scenarios**:
  ```
  Scenario: CCD solve
    Tool: Bash
    Steps:
      1. Create 3-joint arm skeleton
      2. Solve IK with CCD method
    Expected Result: End effector reaches target
    Evidence: .omo/evidence/task-24-ccd-solve.log
  ```

---

### Wave 7: Integration + Backend Extensions

- [ ] 26. **Skin binding + IK→animation integration**

  **What to do**:
  - Implement skin binding system matching Python's `_merge_skin_bindings`:
    - Framework hook registered on timeline
    - Before each frame: evaluate IK → update joint positions → map joint transforms to graphic object parameters
    - Skin binding maps: skeleton joint name → graphic object id + parameter path
  - `_skin_bind()` PML builtin: `(skin-bind skeleton-instance joint-name graphic-object :param "transform")`
  - Frame hook pipeline: IK solve → update skeleton → apply skin bindings → modify graphic objects
  - Integration with Timeline.render_frames()

  **Must NOT do**:
  - No new IK algorithms (reuse FABRIK/CCD from tasks 23, 24)

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: NO (integration task)
  - **Parallel Group**: Wave 7 (with Tasks 26, 27)
  - **Blocks**: Animation→GIF pipeline
  - **Blocked By**: Tasks 21, 23, 24 (Timeline + IK solvers)

  **Acceptance Criteria**:
  - [ ] Skin binding maps joint transforms to graphic objects
  - [ ] Frame hooks call IK solve before rendering
  - [ ] Animated frames reflect both animation and IK modifications

  **QA Scenarios**:
  ```
  Scenario: Skin binding + IK
    Tool: Bash
    Steps:
      1. Create skeleton, bind to graphic
      2. Set IK target, render frame
      3. Verify graphic object reflects joint position
    Expected Result: Graphic moves according to IK solution
    Evidence: .omo/evidence/task-25-skin-binding.png
  ```

- [ ] 27. **Spritesheet rendering**

  **What to do**:
  - Implement spritesheet rendering via `_render_spritesheet()`
  - Port `pml/graphics/render.py:_render_spritesheet` exactly
  - Grid layout: cols, cell_width, cell_height, padding parameters
  - Render each sprite onto cell-sized sub-surface
  - Paste sub-surfaces onto grid canvas
  - Generate metadata JSON matching Python format:
    - file, format, total_width, total_height, cols, rows, cell_width, cell_height, padding, frames[]
  - `register_render()` already registered from Task 13; extend with spritesheet

  **Must NOT do**:
  - No new grid layout algorithms (match Python behavior)

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 7 (with Tasks 25, 26)
  - **Blocks**: CLI rendering
  - **Blocked By**: Tasks 12, 13 (SkiaBackend + Render dispatch)

  **Acceptance Criteria**:
  - [ ] Spritesheet renders correctly with N sprites in grid
  - [ ] Metadata JSON generated
  - [ ] Output matches Python spritesheet on test case

  **QA Scenarios**:
  ```
  Scenario: Spritesheet
    Tool: Bash
    Steps:
      1. Create 4 sprites, render-spritesheet
      2. Verify grid layout and metadata
    Expected Result: Correct spritesheet + meta.json
    Evidence: .omo/evidence/task-27-spritesheet.png
  ```

---

- [ ] 28. **BackendRegistry PML builtins (list-backends, set-backend!, etc.)**

  **What to do**:
  - Implement PML-level backend management builtins in `src/pml/builtins/backend_builtins.cpp`
  - `(list-backends)` → list of backend info (name, description, capabilities)
  - `(set-backend! "name")` → switch active backend, errors if unknown
  - `(current-backend)` → name of active backend
  - `(backend-capabilities)` → list of capabilities for active backend
  - `(backend-available? "name")` → bool
  - `(create-surface w h [bg])` → surface reference (for explicit backend use)
  - Query `BackendRegistry::available()` / `active()` / `create_best()`
  - Register via `register_backend_builtins(env)` in PMLRuntime startup

  **Must NOT do**:
  - No backend creation without selection (always use set-backend! first)
  - No .so plugin loading

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 7 (with Tasks 26, 27, 29)
  - **Blocks**: CLI, REPL backend features
  - **Blocked By**: Tasks 11, 12, 13 (BackendRegistry + SkiaBackend + Render dispatch)

  **Acceptance Criteria**:
  - [ ] `(list-backends)` returns ["skia", "null"] when both registered
  - [ ] `(set-backend! "null")` switches to NullBackend
  - [ ] `(current-backend)` returns current backend name
  - [ ] `(backend-capabilities)` returns correct BackendCap values
  - [ ] `(backend-available? "skia")` returns #t

  **QA Scenarios**:
  ```
  Scenario: Backend PML builtins
    Tool: Bash
    Steps:
      1. Register mock backend, call (list-backends)
      2. (set-backend! "null"), (current-backend) → "null"
    Expected Result: Backend builtins work correctly
    Evidence: .omo/evidence/task-28-backend-builtins.log
  ```

- [ ] 29. **Shader subsystem (SkRuntimeEffect wrapper + (shader "...") builtin)**

  **What to do**:
  - Implement shader support in `src/pml/backend/skia/shader.h/.cpp`
  - `Shader` class wrapping `sk_sp<SkRuntimeEffect>`:
    - `compile(glsl_src)` → Result<Shader> (compile error → PML error)
    - `uniforms()` → list of uniform names/types from SkRuntimeEffect
    - `make_shader(uniforms, local_matrix)` → `sk_sp<SkShader>`
  - `compile_shader()` method on RenderBackend ABC (optional, default returns error)
  - PML builtin `(shader "...glsl...")`:
    - Accepts inline GLSL string
    - Calls `BackendRegistry::active().compile_shader(glsl)`
    - Returns shader handle usable in draw calls
  - PML builtin `(apply-shader! graphic-object shader-handle [uniform-map])`:
    - Binds shader to a shape's fill/stroke for next render
    - `Shader` stored in GraphicObject's metadata
  - Skia specific: `SkRuntimeEffect::MakeForShader()` compiles GLSL
  - Graceful fallback: if backend doesn't support shaders, return clear error
  - Allow rendering fallback (no-shader mode) — shader is optional enhancement

  **Must NOT do**:
  - No custom GLSL compilation (delegate to SkRuntimeEffect entirely)
  - No vertex/fragment shaders — Skia fragment shader only (SkRuntimeEffect)
  - Shader is optional — all rendering must work without it

  **Recommended Agent Profile**:
  - **Category**: `visual-engineering`
  - **Skills**: N/A

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 7 (with Tasks 26-28)
  - **Blocks**: Advanced rendering demos
  - **Blocked By**: Tasks 12, 15 (SkiaBackend + Timeline — Skia needed for compile_shader)

  **Acceptance Criteria**:
  - [ ] `(shader "half4 main(vec2 fragCoord) { return half4(1,0,0,1); }")` compiles
  - [ ] `apply-shader!` attaches shader to graphic object
  - [ ] Rendering with shader produces correctly shaded output
  - [ ] Backend without shader support returns clear error
  - [ ] GLSL compile error returns meaningful PML error message

  **QA Scenarios**:
  ```
  Scenario: Shader compilation
    Tool: Bash
    Steps:
      1. Compile simple red fragment shader
      2. Apply to circle, render
      3. Compare output with solid red circle
    Expected Result: Shader produces equivalent visual output
    Evidence: .omo/evidence/task-29-shader-basic.png
  ```

---

### Wave 8: CLI + MCP + API

- [x] 30. **PMLRuntime API facade**

  **What to do**:
  - Implement `src/pml/api.h/.cpp`
  - Port `pml/api.py` exactly
  - `PMLRuntime` class:
    - Constructor creates global environment
    - Loads builtins, transforms, graphics, sprites, module system
    - `execute(source, filename="<stdin>")` → Result<RenderResult>
      - Tokenize → Parse → Expand → Evaluate
      - Return files, value, or error
    - `execute_file(path)` → Result<RenderResult>
    - `execute_pml(source, options)` → dict matching Python RenderResult format
    - Error formatting matching Python's `_error_to_dict`
  - `RenderResult` struct: success, value (optional Value), files (vector<string>), error (optional Error dict)

  **Must NOT do**:
  - Must produce identical JSON output structure as Python API

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 8 (with Tasks 29-31)
  - **Blocks**: CLI, MCP (both depend on this)
  - **Blocked By**: Tasks 15, 16 (Evaluator + Builtins)

  **Acceptance Criteria**:
  - [ ] `execute("(+ 1 2)")` → RenderResult{success=true, value=3}
  - [ ] Error results match Python format exactly
  - [ ] execute_file loads, runs, and returns correct results
  - [ ] JSON output matches Python `to_json()`

  **QA Scenarios**:
  ```
  Scenario: PMLRuntime execute
    Tool: Bash
    Steps:
      1. call execute("(canvas 100 100)(rect 10 10 50 50 :fill \"red\")(render \"/tmp/out.png\")")
      2. Verify PNG file created
    Expected Result: Rendering works through API
    Evidence: .omo/evidence/task-28-api-execute.png
  ```

- [ ] 31. **CLI (file exec, REPL, --watch, --json, -o)**

  **What to do**:
  - Implement `src/pml/cli/main.cpp` and `repl.h/.cpp`
  - Port `pml/repl.py` exactly
  - CLI flags: `pml [file] [-o output_dir] [--json] [--watch] [--gif] [--help]`
  - File execution mode:
    - Tokenize → Parse → Expand → Evaluate
    - Render output to files (via -o dir or current dir)
    - Print results with --json flag
  - REPL mode (no file argument):
    - linenoise-based readline loop
    - Read → Evaluate → Print
    - Track __source_file__ for error reporting
  - --watch mode:
    - inotify-based file monitoring (Linux)
    - Auto re-run on file change
  - Output directory handling: -o flag creates dir and redirects render output
  - Error output formatting matching Python

  **Must NOT do**:
  - No Windows file monitoring (V2)

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 8 (with Tasks 28, 30, 31)
  - **Blocks**: Testing
  - **Blocked By**: Task 28 (PMLRuntime)

  **Acceptance Criteria**:
  - [ ] `./build/pml -e "(+ 1 2)"` outputs "3"
  - [ ] `./build/pml examples/hello.pml` renders PNG to current dir
  - [ ] `./build/pml examples/hello.pml -o /tmp/out` renders to /tmp/out
  - [ ] `./build/pml --json` produces JSON output
  - [ ] REPL starts and evaluates expressions
  - [ ] --watch re-runs on file change

  **QA Scenarios**:
  ```
  Scenario: CLI file execution
    Tool: Bash
    Steps:
      1. ./build/pml examples/hello.pml -o /tmp/pml-test
      2. Check output file exists
    Expected Result: PNG file generated
    Evidence: .omo/evidence/task-29-cli-exec.png

  Scenario: CLI JSON output
    Tool: Bash
    Steps:
      1. ./build/pml -e "(+ 1 2)" --json
    Expected Result: {"success": true, "value": 3, ...}
    Evidence: .omo/evidence/task-29-cli-json.log
  ```

- [ ] 32. **MCP server**

  **What to do**:
  - Implement `src/pml/mcp/mcp_server.h/.cpp`
  - Port `pml/mcp_server.py` exactly
  - JSON-RPC 2.0 over stdio with `Content-Length: N\r\n\r\n` framing
  - 5 tools with identical names and schemas:
    1. `execute_pml` — run PML source code, return results
    2. `render_sprite` — render a character sprite with params
    3. `validate` — validate PML source without executing
    4. `list_components` — list available sprite components
    5. `preview_params` — get parameter info for a component
  - Tool discovery via `tools/list` and `tools/call`
  - Uses nlohmann/json for JSON handling
  - Responds to `initialize` handshake
  - Error responses match Python MCP format

  **Must NOT do**:
  - No WebSocket transport (stdio only)
  - No authentication

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 8 (with Tasks 28, 29, 31)
  - **Blocks**: Testing
  - **Blocked By**: Task 28 (PMLRuntime)

  **Acceptance Criteria**:
  - [ ] `echo 'Content-Length: N\n\n{"jsonrpc":"2.0","method":"tools/list","id":1}' | ./build/pml-mcp` returns 5 tools
  - [ ] `tools/call execute_pml {"source": "(+ 1 2)"}` returns 3
  - [ ] Error responses have correct JSON-RPC error structure
  - [ ] All 5 tools match Python MCP schemas exactly

  **QA Scenarios**:
  ```
  Scenario: MCP tools/list
    Tool: Bash (pipe JSON-RPC to pml-mcp)
    Steps:
      1. Send initialize + tools/list via stdio
      2. Parse response
    Expected Result: 5 tools returned matching Python
    Evidence: .omo/evidence/task-30-mcp-tools-list.log
  ```

- [x] 33. **Stdlib runtime loader**

  **What to do**:
  - Implement stdlib loading in `src/pml/module/embedded_stdlib.h/.cpp`
  - On `PMLRuntime` initialization:
    - Iterate embedded stdlib file entries
    - For each file: parse module name from file path (matching Python `Path(path).stem`)
    - Load each .pml module into the global environment
    - Register standard library modules: math, color, easing, shapes
    - Handle errors gracefully (missing symbols, load failures)
  - Integration with module system (Task 17)

  **Must NOT do**:
  - Don't modify stdlib .pml content (byte-identical embedding)

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 8 (with Tasks 28-30)
  - **Blocks**: Testing
  - **Blocked By**: Tasks 6, 17 (embedding + module system)

  **Acceptance Criteria**:
  - [ ] All stdlib modules load successfully
  - [ ] `(math/sin 0)` returns 0.0
  - [ ] `(color/rgb 255 0 0)` returns color string
  - [ ] Module names match Python `Path.stem` output

  **QA Scenarios**:
  ```
  Scenario: Stdlib loading
    Tool: Bash
    Steps:
      1. Initialize PMLRuntime
      2. Call (math/sin 0)
    Expected Result: 0.0
    Evidence: .omo/evidence/task-31-stdlib-load.log
  ```

---

### Wave 9: Testing + Parity

- [ ] 34. **Per-module Google Test suite**

  **What to do**:
  - Create Google Test files for each module in `tests/`:
    - `test_lexer.cpp` — match `tests/test_lexer.py` (25 tests)
    - `test_parser.cpp` — match `tests/test_parser.py` (26 tests)
    - `test_evaluator.cpp` — match `tests/test_evaluator.py` + `test_phase4.py` (57+77=134 tests)
    - `test_graphics.cpp` — match `tests/test_graphics_sprites.py` + `test_phase5.py` (48+50=98 tests)
    - `test_phase6.cpp` — match `tests/test_phase6.py` (77 tests)
    - `test_phase7.cpp` — match `tests/test_phase7.py` (77 tests)
    - `test_phase8.cpp` — match `tests/test_phase8.py` (68 tests)
  - Each test file registered in `tests/CMakeLists.txt`
  - Test fixtures for common setup (environment, builtins)
  - Target: 389+ tests passing
  - CMake target: `pml_tests` — `ctest --test-dir build` runs all

  **Must NOT do**:
  - No mock frameworks unless absolutely necessary (test against real implementations)
  - Don't skip tests for "minor" features

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 9 (with Tasks 33, 34)
  - **Blocks**: Verification
  - **Blocked By**: All implementation tasks

  **Acceptance Criteria**:
  - [ ] All tests compile and link to gtest
  - [ ] `ctest --test-dir build` returns exit code 0
  - [ ] Test count matches or exceeds 389

  **QA Scenarios**:
  ```
  Scenario: Full test suite
    Tool: Bash
    Steps:
      1. cmake --build build --target pml_tests
      2. ctest --test-dir build --output-on-failure
    Expected Result: All tests pass (389+)
    Evidence: .omo/evidence/task-32-test-suite.log
  ```

- [ ] 35. **Parity verification script**

  **What to do**:
  - Create Python script `tools/parity_check.py`
  - For each .pml file in `examples/` and key test fixtures:
    1. Run with Python PML: `uv run pml {file} -o /tmp/pml-py/`
    2. Run with C++ PML: `./build/pml {file} -o /tmp/pml-cpp/`
    3. Compare outputs:
       - PNG: pixel-level comparison (ImageMagick compare or Python Pillow)
       - JSON output: parse and compare structure
       - GIF: frame count, dimensions, per-frame pixel comparison
       - Error cases: error message content, line/column numbers
  - 4. Report: "ALL PASS" or "MISMATCH: file.png differs at pixel (x,y)"
  - Optionally add to CMake as `check-parity` target
  - Threshold: non-font-rendering operations should match pixel-perfectly

  **Must NOT do**:
  - Don't modify .pml test files (run as-is)

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 9 (with Tasks 32, 34)
  - **Blocks**: Verification
  - **Blocked By**: All implementation tasks

  **Acceptance Criteria**:
  - [ ] Script produces "ALL PASS" exit code 0 for matching outputs
  - [ ] Script reports differences with exact file and pixel location
  - [ ] Can be run as `cmake --build build --target check-parity`

  **QA Scenarios**:
  ```
  Scenario: Parity check
    Tool: Bash
    Steps:
      1. python tools/parity_check.py --pml-py /path/to/python/pml --pml-cpp ./build/pml
    Expected Result: exit code 0, "ALL PASS"
    Evidence: .omo/evidence/task-33-parity.log
  ```

- [ ] 36. **BEHAVIOR_DIFFERENCES.md**

  **What to do**:
  - Create `BEHAVIOR_DIFFERENCES.md` in project root
  - Document all known, intentional differences between Python and C++ versions:
    - Font rendering differences (Skia vs Pillow)
    - GIF palette reduction (minor color variations)
    - Floating-point rounding in edge cases
    - Any other known differences
  - For each difference: describe what differs, why, and whether it's acceptable
  - Reference this file from README.md

  **Must NOT do**:
  - Don't document unintentional bugs (should fix those)

  **Recommended Agent Profile**:
  - **Category**: `writing`

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 9 (with Tasks 32, 33)
  - **Blocks**: Final verification
  - **Blocked By**: All implementation tasks

  **Acceptance Criteria**:
  - [ ] Document covers all known differences
  - [ ] Each entry has: what differs, magnitude, reason, acceptance decision
  - [ ] Referenced from README

  **QA Scenarios**:
  ```
  Scenario: Document review
    Tool: Bash
    Steps:
      1. Read BEHAVIOR_DIFFERENCES.md
    Expected Result: All known differences documented
    Evidence: .omo/evidence/task-34-diff-doc.log
  ```
