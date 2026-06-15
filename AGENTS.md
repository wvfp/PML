# PML (Picture Markup Language) — Agent Guide

## Project overview

PML is a Lisp-style DSL for sprite asset generation. It compiles S-expressions through a hand-written pipeline (lex → parse → expand → evaluate → render) and outputs game-ready sprite PNGs via Pillow.

**Entry points**: `pml` (CLI, via `pml.repl:main`) or `python -m pml`

## Dev commands

```bash
# Run all tests
uv run pytest

# Run a specific test file
uv run pytest tests/test_lexer.py

# Run a specific test class or method (use -k for partial match)
uv run pytest tests/test_evaluator.py -k "TestArithmetic"

# Run the interpreter on a file
uv run python -m pml examples/hello.pml

# Start REPL
uv run pml

# Lint
uv run ruff check .

# Type-check (no mypy/pyright config in project — but ruff lint covers basic issues)
```

**Important**: Always prefix with `uv run` — the project uses `uv` as its package manager. Never use bare `pytest` or bare `ruff`.

## Project structure

```
pml/                          # Core interpreter package
├── __main__.py               # python -m pml entry
├── repl.py                   # CLI entry + create_global_env()
├── api.py                    # PMLRuntime — LLM-facing API (validation, execution, component discovery)
├── lexer.py → parser.py → expander.py → evaluator.py  # Pipeline order
├── environment.py            # Lexical scoping with parent chain
├── module_loader.py          # import / provide system with caching & circular dep detection
├── errors.py                 # All exception types
├── types.py                  # Symbol, Keyword, Procedure, BuiltinProcedure, Macro
├── transform.py              # AffineTransform (compose, inverse, apply)
├── transform_builtins.py     # PML bindings for transform ops
├── builtins/                 # Core builtins: arithmetic, comparison, io, list_ops, string_ops, type_predicates
├── graphics/                 # Primitives (circle/rect/...), canvas, render, objects
├── backend/                  # RenderBackend ABC + Pillow backend
├── sprites/                  # Style system, palette, component registry + components/ (character, body, eyes, hair, mouth, head, outfit, items, ui_widgets, scene_elements)
├── animation/                # Animation engine, easing, interpolate, timeline
└── skeleton/                 # SkeletonTemplate, SkeletonInstance, IK solvers (FABRIK, CCD), evaluator
stdlib/                       # Standard library (.pml files)
├── math.pml                  # clamp, lerp, normalize, remap, distance, sign
├── color.pml                 # Color utilities
├── easing.pml                # Easing functions
├── shapes.pml                # centered-rect, diamond, arrow
└── sprites/                  # Component API docs (.pml): character, body, eyes, hair, outfit, items, ui, scene, styles
tests/                        # pytest test files
├── test_lexer.py             # Phase 0
├── test_parser.py            # Phase 0
├── test_evaluator.py         # Phase 1 (arithmetic, variables, functions, closures, macros, modules)
├── test_graphics_sprites.py  # Phase 2+3 (transforms, graphics, rendering, sprites)
├── test_phase4.py            # Extended semantic components (outfit, items, UI, scene)
├── test_phase5.py            # Animation system
├── test_phase6.py            # Timeline
├── test_phase7.py            # Graphics/sprites integration
└── test_phase8.py            # Skeleton + IK (FABRIK, CCD, bind-skin)
examples/                     # Runnable .pml demos
```

## Architecture notes

**Pipeline order (non-negotiable)**: `Lexer(source) → Parser(tokens) → Expander(env).expand_all(ast) → evaluator.evaluate(expr, env)`. The Expander runs macro expansion as a pre-pass — it must come before evaluation.

**Environment chain**: `Environment` has a `parent` pointer. `define()` binds in current scope, `set!` searches upward, `lookup` searches outward through parents, `extend()` creates child scopes for function calls.

**Global singletons** (watch for state leakage):
- `Timeline` (animation) — `Timeline.get_current()` returns a thread-local singleton
- `SpriteCanvas` (sprites) — managed via `sprite-canvas` / `canvas` / `add` / `render`
- `_skin_bindings` (skeleton) — module-level dict in `pml/skeleton/__init__.py`, call `clear_skin_bindings()` in test setup

**Special forms** (defined in `evaluator.py` via `SPECIAL_FORMS` dict): `quote`, `if`, `cond`, `define`, `lambda`, `begin`, `set!`, `let`, `let*`, `do`, `and`, `or`, `defmacro`, `defskeleton`, `provide`, `import`, `load`. These are NOT regular functions — they control evaluation of their arguments.

**`defskeleton` is special**: It's registered via `SPECIAL_FORMS["defskeleton"] = _eval_defskeleton` in `pml/skeleton/__init__.py`, because joint keywords like `:pos` must not be evaluated as regular arguments.

**Module system**: Each `.pml` file is a module. `provide` exports symbols, `import` loads and caches. `prefix/symbol` syntax accesses exported symbols. Circular imports detected via `loading` set.

**Testing patterns**: Tests use a local `run()` helper that differs across test files:
- `test_evaluator.py`: `run(source)` returns `(result, env)` — does NOT run Expander (macros tested separately)
- `test_phase8.py`: `_eval(source)` DOES run Expander before evaluation
- `test_graphics_sprites.py`: `run(source)` does NOT run Expander

When adding tests, use the helper from the closest test file. For integration tests that mix macros with evaluation, always run Expander first.

## Config & tooling

- **Python**: 3.13 (`.python-version`), min 3.10 (`pyproject.toml`)
- **Build**: hatchling
- **Runtime dep**: pillow>=12.2.0
- **Dev deps**: pytest>=8.0, ruff>=0.4
- **Lint**: ruff, select E/F/W/I, line-length=100
- **No type-checker config** (no mypy/pyright in project)
- **No CI** workflows
- **No pre-commit hooks**

## Style conventions

- `from __future__ import annotations` at top of every `.py` file
- `__slots__` on performance-critical classes
- `@dataclass(slots=True)` for data types
- Type hints throughout (no `# type: ignore` without explicit reason)
- Error types in `pml/errors.py`, all inherit from `PMLError`
- Builtin registration pattern: a `register()` function that receives `Environment` and calls `env.define(name, BuiltinProcedure(...))`
- Component registration pattern: `@sprite_component("name")` decorator (in `sprites/registry.py`)

## Common pitfalls for agents

1. **Always use `uv run`**, not bare `python` or `pytest`. The `.venv` is uv-managed.
2. **Stdlib paths**: `module_loader.py` searches `PML_PATH` env var, then `stdlib/` relative to the `pml/` package root. The `stdlib/` directory is at the repo root, not inside `pml/`.
3. **Sprite rendering needs `sprite-canvas`** before `add`/`render`. The `canvas` function sets up a basic canvas; `sprite-canvas` sets up sprite-specific framing (anchor, padding). Mixing them incorrectly causes missing output.
4. **Animation runs on `Timeline` singleton** — `render` with `:fps` triggers frame generation via `Timeline.render_frames()`. Without calling `play()` after adding animations, nothing animates.
5. **GraphicObject is frozen** — `with_transform()` and `with_param()` return new instances. Never mutate in place.
6. **`every-frame` hooks** are called during `render_frames()`, not during evaluation. Test IK by calling `ik-solve` and inspecting joint angles directly.
7. **No git commits yet** — `git init` was run but `master` has zero commits. Be careful with git operations.
