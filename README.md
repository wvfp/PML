# PML — Picture Markup Language

**A Lisp-style DSL for code-to-image generation.** Write images in S-expressions, compile to PNG/GIF via Pillow. Built for LLM agents — describe what you want in code, get the image back.

```lisp
(sprite-canvas 128 256 :bg "transparent")
(add (character :expression "happy" :style 'cel))
(render "hero.png")
```

---

## Quick Start

```bash
# Install
uv sync

# Run a file
uv run pml examples/hello.pml

# Start REPL
uv run pml

# Render a sprite (see examples/sprite.pml)
uv run pml examples/sprite.pml -o examples_output

# Render animation as GIF
uv run pml examples/animation.pml -o examples_output

# CLI options
uv run pml file.pml --json          # Structured JSON output
uv run pml file.pml -o ./output     # Set output directory
uv run pml file.pml --watch         # Auto-re-run on file change
uv run pml --help                   # All options
```

---

## Examples

### Basic Graphics

```lisp
(canvas 300 200 :bg "#f5f5f5")
(add (rect 20 20 80 60 :fill "#3498db" :stroke "#2c3e50" :stroke-width 2))
(add (circle 180 60 35 :fill "#e74c3c"))
(add (ellipse 260 100 25 40 :fill "#2ecc71"))
(render "shapes.png")
```

### Character Sprite

```lisp
(sprite-canvas 128 256 :bg "transparent" :anchor 'center-bottom)

(define hero
  (character
    :outfit (outfit :top 'armor :bottom 'armor :shoes 'boots)
    :weapon (weapon :type 'sword :material 'crystal :element 'holy)
    :expression "happy"
    :style 'cel))

(add hero)
(render "hero.png")
```

### Animation → GIF

```lisp
(canvas 200 200 :bg "#1a1a2e")

(define ball (circle 30 30 20 :fill "#e74c3c"))

(animate ball 'x 30 170 1.0 :ease 'sine-in-out)
(animate ball 'y 30 170 0.4 :ease 'bounce-out)
(play)
(render "bounce.gif" :fps 30)
```

### Skeleton & IK

```lisp
(defskeleton arm (x y)
  (joint shoulder :pos (0 0)   :length 40.0 :angle 0.0 :min -120 :max 120)
  (joint elbow   :pos (0 40)  :length 35.0 :angle 0.0 :min -150 :max 0)
  (joint wrist   :pos (0 35)  :length 0.0  :angle 0.0))

(define arm-inst (instantiate-skeleton arm 50 180))
(ik-solve arm-inst 'wrist 95 125 :method 'fabrik :iterations 30)
(joint-position arm-inst 'elbow)   ; → (x y)
```

### Spritesheet

```lisp
(render-spritesheet "sprites.png"
  (character :expression "happy") (character :expression "angry")
  :cols 2 :cell-width 64 :cell-height 64)
; → spritesheet.png + spritesheet.spritesheet.json
```

### UI Widgets

```lisp
(sprite-canvas 256 192)
(define btn (button :label "Start" :width 100 :height 32 :style 'rounded))
(add (group :transform (translate 0 10) btn))
(render "ui.png")
```

More examples in [`examples/`](examples/).

---

## Features

- **Full Lisp dialect**: S-expressions, closures, macros, lexical scoping
- **Graphics pipeline**: primitives, transforms, canvas compositing
- **Semantic component system**: style engine, palette management, reusable components (characters, UI widgets, scene elements, items, etc.)
- **Animation engine**: property tweening, timeline, easing functions, GIF export
- **Inverse Kinematics**: FABRIK and CCD solvers with angle constraints and skin binding
- **Module system**: import/provide with caching and circular dependency detection
- **Standard library**: math, color, easing, shapes
- **MCP server**: expose as tools for AI agents (Claude, Cursor)
- **CLI**: file execution, REPL, watch mode, JSON output, output directory control
- **Pillow backend**: PNG, JPG, GIF output
- **LLM-friendly API**: structured error hints for AI repair

---

## Project Structure

```
pml/                          # Core interpreter
├── lexer.py → parser.py → expander.py → evaluator.py  # Pipeline
├── environment.py            # Lexical scoping
├── module_loader.py          # import/provide system
├── errors.py                 # Error types with repair hints
├── types.py                  # Symbol, Keyword, Procedure, Macro
├── repl.py                   # CLI + REPL entry
├── api.py                    # PMLRuntime — LLM-facing API
├── mcp_server.py             # MCP server for AI agents
├── transform.py              # AffineTransform
├── builtins/                 # Arithmetic, comparison, IO, list/string ops
├── graphics/                 # Primitives, canvas, render
├── sprites/                  # Style, palette, components
├── animation/                # Timeline, easing, interpolation
├── skeleton/                 # SkeletonTemplate, FABRIK/CCD IK
└── backend/                  # Pillow render backend
stdlib/                       # Standard library (.pml files)
├── math.pml, color.pml, easing.pml, shapes.pml
└── sprites/                  # Component API docs
examples/                     # Runnable .pml demos
tests/                        # pytest test suite (389 tests)
docs/
├── language.md               # Full language reference
└── README.zh.md              # Chinese guide
```

---

## Documentation

| Resource | Link |
|----------|------|
| Language Reference | [`docs/language.md`](docs/language.md) |
| Chinese Guide | [`docs/README.zh.md`](docs/README.zh.md) |
| Examples | [`examples/`](examples/) |
| API Reference | [`pml/api.py`](pml/api.py) |

---

## MCP Server

For AI agents (Claude Desktop, Cursor, etc.):

```bash
uv run pml-mcp
```

Exposes 5 tools: `execute_pml`, `render_sprite`, `validate`, `list_components`, `preview_params`.

Add to Claude Desktop config:
```json
{
  "mcpServers": {
    "pml": {
      "command": "uv",
      "args": ["run", "pml-mcp"],
      "cwd": "/path/to/PML"
    }
  }
}
```

---

## Requirements

- Python ≥ 3.10
- Pillow ≥ 12.2.0
- `uv` package manager ([install guide](https://docs.astral.sh/uv/#installation))

---

## License

[MIT](LICENSE)
