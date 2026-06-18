# PML Language Reference

PML (Picture Markup Language) is a Lisp-style DSL for code-to-image generation. It compiles S-expressions through a pipeline (lex → parse → expand → evaluate → render) and outputs PNG/GIF images via Pillow. Built for LLM agents — describe any visual in code, get the image back.

---

## 1. Getting Started

```bash
# Run a .pml file
uv run pml examples/hello.pml

# Start REPL
uv run pml

# CLI options
uv run pml --help
uv run pml file.pml --json          # Structured JSON output
uv run pml file.pml -o ./output     # Set output directory
uv run pml file.pml --watch         # Auto-re-run on file change

# MCP server (for AI agent integration)
uv run pml-mcp
```

---

## 2. Syntax

PML uses S-expressions (prefix notation). Everything is either an atom or a list.

### Data Types

| Type | Example | Notes |
|------|---------|-------|
| Integer | `42`, `-5` | 64-bit |
| Float | `3.14`, `-0.5` | |
| Boolean | `#t`, `#f` | |
| String | `"hello"`, `"path/to/file.png"` | Double-quoted |
| Symbol | `hello`, `+`, `my-variable` | Unquoted identifiers |
| Keyword | `:key`, `:fill`, `:width` | Prefixed with `:`, used as parameter markers |
| List | `(1 2 3)`, `(+ 1 2)` | Function call or data |
| Quoted list | `'(1 2 3)` | Data (not evaluated) |
| `nil` / empty list | `()` | Falsy |

**Important**: `(1 2 3)` without quote is a function call — it tries to call `1` as a function with args `2 3`. Use `'(1 2 3)` or `(list 1 2 3)` for data lists.

### Comments

```lisp
; Line comment starts with semicolon

;; Double semicolons for section headers
```

---

## 3. Special Forms

Special forms control evaluation of their arguments (not all arguments are evaluated).

### `quote`

```lisp
(quote expr)      ; Returns expr unevaluated
'expr             ; Shorthand for (quote expr)

'(1 2 3)          ; → (1 2 3) — a list, not a function call
'my-symbol        ; → my-symbol (as a Symbol, not a variable lookup)
```

### `if`

```lisp
(if condition then-expr else-expr?)  ; else-expr is optional

(if (> x 10) "big" "small")
(if (< n 0) "negative")              ; returns None if condition falsy
```

### `cond`

```lisp
(cond
  (test1 expr1 ...)
  (test2 expr2 ...)
  (else expr-default ...))

(cond
  ((< n 0) "negative")
  ((= n 0) "zero")
  (else "positive"))
```

### `define`

```lisp
(define name value)         ; Variable definition
(define (name params...) body...)  ; Function shorthand

(define pi 3.14159)
(define (square x) (* x x))
(define (add a b) (+ a b))
```

### `lambda`

```lisp
(lambda (params...) body...)

(lambda (x) (* x x))
(map (lambda (x) (+ x 1)) '(1 2 3))  ; → (2 3 4)
```

### `begin`

```lisp
(begin expr1 expr2 ...)     ; Evaluate sequentially, return last value

(begin
  (println "step 1")
  (println "step 2")
  42)
```

### `set!`

```lisp
(set! name value)           ; Mutate existing binding (searches upward)

(define x 10)
(set! x 20)                 ; x is now 20
```

### `let` / `let*`

```lisp
(let ((name1 expr1) (name2 expr2) ...) body...)   ; Parallel bindings
(let* ((name1 expr1) (name2 expr2) ...) body...)  ; Sequential bindings

(let ((x 1) (y 2)) (+ x y))                       ; → 3
(let* ((x 1) (y (+ x 1))) y)                      ; → 2
```

### `and` / `or`

```lisp
(and expr1 expr2 ...)       ; Short-circuit, return last truthy or first falsy
(or expr1 expr2 ...)        ; Short-circuit, return first truthy

(and #t 42 "hello")         ; → "hello"
(or #f 0 42)                ; → 42
```

### `do` (iteration)

```lisp
(do ((var init step) ...) (test result) body...)

(do ((i 0 (+ i 1))) ((= i 5) i)
  (println i))
```

### `quasiquote` / `unquote`

```lisp
(quasiquote expr)           ; Template with unquote interpolation
`expr                       ; Shorthand

(set! x 42)
`(x is ,x)                  ; → (x is 42)
```

### `defmacro`

```lisp
(defmacro name (params...) body...)

(defmacro unless (cond body)
  `(if (not ,cond) ,body))

(unless #f (println "runs"))  ; prints "runs"
```

### `provide` / `import`

```lisp
(provide sym1 sym2 ...)     ; Declare exported symbols

(import "path.pml" as prefix)  ; Load module with prefix
(import "path.pml" prefix)     ; Shorthand without 'as'

;; Access exports: prefix/exported-name
(import "math.pml" as math)
(math/clamp 0.5 0 1)
```

### `defskeleton`

```lisp
(defskeleton name (param-x param-y)
  (joint joint-name
    :pos (dx dy)
    :length len
    :angle angle
    :min min-angle?    ; optional
    :max max-angle?)   ; optional
  ...)

;; Example:
(defskeleton arm (x y)
  (joint shoulder :pos (0 0) :length 40.0 :angle 0.0 :min -120 :max 120)
  (joint elbow   :pos (0 40) :length 35.0 :angle 0.0 :min -150 :max 0)
  (joint wrist   :pos (0 35) :length 0.0  :angle 0.0))
```

---

## 4. Built-in Functions

### Arithmetic

| Function | Description | Example |
|----------|-------------|---------|
| `(+ a b ...)` | Addition | `(+ 1 2 3)` → `6` |
| `(- a b ...)` | Subtraction / negation | `(- 10 3)` → `7`, `(- 5)` → `-5` |
| `(* a b ...)` | Multiplication | `(* 2 3 4)` → `24` |
| `(/ a b ...)` | Division | `(/ 10 3)` → `3.333` |
| `(mod a b)` | Modulo | `(mod 10 3)` → `1` |
| `(abs n)` | Absolute value | `(abs -5)` → `5` |
| `(floor n)` | Round down | `(floor 3.7)` → `3` |
| `(ceil n)` | Round up | `(ceil 3.2)` → `4` |
| `(round n)` | Round nearest | `(round 3.5)` → `4` |
| `(sqrt n)` | Square root | `(sqrt 16)` → `4` |
| `(pow b e)` | Power | `(pow 2 3)` → `8` |
| `(min a b ...)` | Minimum | `(min 3 7 1)` → `1` |
| `(max a b ...)` | Maximum | `(max 3 7 1)` → `7` |
| `(pi)` | Pi constant | → `3.14159` |
| `(sin n)`, `(cos n)`, `(tan n)` | Trigonometry | Radians |
| `(asin n)`, `(acos n)`, `(atan n)` | Inverse trig | |
| `(rand)` | Random 0..1 | `(rand)` |

### Comparison

| Function | Description |
|----------|-------------|
| `(= a b)` | Equal (numeric) |
| `(!= a b)` | Not equal |
| `(< a b)`, `(> a b)`, `(<= a b)`, `(>= a b)` | Ordering |
| `(eq? a b)` | Identity (same object) |
| `(equal? a b)` | Structural equality |

### List Operations

| Function | Description | Example |
|----------|-------------|---------|
| `(car lst)` | First element | `(car '(1 2 3))` → `1` |
| `(cdr lst)` | Rest of list | `(cdr '(1 2 3))` → `(2 3)` |
| `(cons a b)` | Prepend | `(cons 1 '(2 3))` → `(1 2 3)` |
| `(list a b ...)` | Create list | `(list 1 2 3)` |
| `(length lst)` | List length | `(length '(1 2 3))` → `3` |
| `(append a b)` | Concatenate | `(append '(1) '(2))` → `(1 2)` |
| `(reverse lst)` | Reverse | |
| `(map fn lst)` | Map function | `(map (lambda (x) (* x 2)) '(1 2 3))` |
| `(filter fn lst)` | Filter | `(filter (lambda (x) (> x 2)) '(1 2 3))` |
| `(reduce fn init lst)` | Reduce | `(reduce + 0 '(1 2 3))` |

### String Operations

| Function | Description |
|----------|-------------|
| `(string? x)` | Is string? |
| `(str-join lst sep)` | Join strings |
| `(str-split s sep)` | Split string |
| `(str-upcase s)` | Uppercase |
| `(str-downcase s)` | Lowercase |
| `(str-replace s old new)` | Replace substring |

### Type Predicates

| Function | Description |
|----------|-------------|
| `(number? x)` | Is number? |
| `(integer? x)` | Is integer? |
| `(boolean? x)` | Is boolean? |
| `(string? x)` | Is string? |
| `(symbol? x)` | Is symbol? |
| `(keyword? x)` | Is keyword? |
| `(list? x)` | Is list? |
| `(procedure? x)` | Is callable? |
| `(null? x)` | Is nil / empty list? |
| `(not x)` | Logical not |

### Input/Output

| Function | Description |
|----------|-------------|
| `(println x ...)` | Print with newline |
| `(print x ...)` | Print without newline |
| `(read)` | Read line from stdin |
| `(gensym prefix?)` | Generate unique symbol |

### Transforms

```lisp
(translate dx dy)               ; Create translation transform
(scale sx sy)                   ; Create scale transform
(rotate angle)                  ; Create rotation transform (radians)
(compose t1 t2)                 ; Compose transforms (t1 then t2)
(inverse t)                     ; Inverse transform
(matrix-apply t x y)            ; Apply transform to point → (x' y')
(identity-matrix)               ; Identity transform
(matrix? t)                     ; Is it an AffineTransform?
```

---

## 5. Graphics

### Canvas

```lisp
;; Regular canvas
(canvas width height :bg "color")

;; Sprite canvas (auto-centering, metadata)
(sprite-canvas width height
  :bg "transparent"             ; Background color (default: transparent)
  :anchor 'center-bottom        ; Anchor point (default: center-bottom)
  :padding 0)                   ; Padding pixels

;; Add objects to canvas
(add graphic-object)
```

### Drawing Primitives

```lisp
(rect x y w h
  :fill "#ff0000"               ; Fill color
  :stroke "#000000"             ; Stroke color
  :stroke-width 1)              ; Stroke width

(circle cx cy r ...)            ; Circle
(ellipse cx cy rx ry ...)       ; Ellipse
(line x1 y1 x2 y2 ...)          ; Line
(polygon (list (list x1 y1) (list x2 y2) ...) ...)  ; Polygon
(path "M10 10 L50 50 Z" ...)   ; SVG-style path
(text x y "string" ...)         ; Text
(image x y :src "file.png")     ; Image
```

All primitives accept `:fill`, `:stroke`, `:stroke-width`.

### Groups

```lisp
(group :transform some-transform child1 child2 ...)

;; Example:
(define g (group :transform (translate 50 50)
           (rect 0 0 20 20 :fill "red")
           (circle 30 30 10 :fill "blue")))
(add g)
```

### Rendered Output

```lisp
;; Static image
(render "output.png")
(render "output.jpg" :format 'jpg)

;; Multi-resolution
(render-set "icon" :content my-object :scales (1 2 4) :base-size (64 64))

;; Spritesheet (grid of sprites)
(render-spritesheet "sprites.png"
  sprite1 sprite2 sprite3 sprite4
  :cols 2
  :cell-width 64
  :cell-height 64
  :padding 2)
;; → Outputs spritesheet PNG + JSON metadata file

;; Animated GIF (requires animations registered on timeline)
(render "anim.gif" :fps 30)
```

Output directory can be controlled globally via `--output-dir` CLI flag.

### Post-Process Effects (Shaders)

Post-process shaders are image-wide filters applied to the canvas at render time, after
all objects have been drawn. They chain in order.

```lisp
;; Apply a shader to the entire canvas
(post-process 'sepia)
(post-process 'blur :radius 3)

;; Multiple shaders chain: grayscale → vignette → noise
(post-process 'grayscale)
(post-process 'vignette :strength 0.5)
(post-process 'noise :amount 0.08 :monochrome #t)
```

**Per-object shader** — apply a filter to a single object before compositing:

```lisp
(define glow-box (shader (rect 10 10 30 30 :fill "red") 'bloom :radius 3))
(add glow-box)
```

**List available shaders:**

```lisp
(list-shaders)
;; → ((name "sepia" type "post-process" description "Warm sepia tone effect") ...)
```

**Define a custom pixel shader** (placeholders for user-defined functions):

```lisp
(define-shader 'my-shader)
```

**Built-in shader reference:**

| Shader | Params | Effect |
|--------|--------|--------|
| `'sepia` | — | Warm vintage tone |
| `'grayscale` | — | Desaturate |
| `'invert` | — | Negative colors |
| `'brightness` | `:factor` (float, default 1.0) | Brighten / darken |
| `'contrast` | `:factor` (float, default 1.0) | Contrast boost |
| `'color-grade` | `:tint` (color), `:strength` (float) | Color tint overlay |
| `'blur` | `:radius` (float, default 2) | Gaussian blur |
| `'sharpen` | `:factor` (float, default 2.0) | Sharpen |
| `'edge-detect` | — | Edge outline extraction |
| `'emboss` | — | Relief emboss effect |
| `'contour` | — | Thin contour outlines |
| `'smooth` | `:iterations` (int, default 1) | Multi-pass smooth/blur |
| `'pixelate` | `:size` (int, default 8) | Blocky pixelation |
| `'vignette` | `:strength` (float, default 0.4) | Corner darkening |
| `'bloom` | `:radius` (float), `:strength` (float) | Glow from bright areas |
| `'oil-paint` | `:range` (int), `:levels` (int) | Painterly / cartoon effect |
| `'crt-scanline` | `:strength` (float, default 0.15) | Retro monitor scanlines |
| `'noise` | `:amount` (float), `:monochrome` (bool) | Film grain texture |

All shaders work on any canvas output. For a full demo run:

```bash
uv run pml examples/shader.pml -o examples_output
```

### Hand-Drawn / Sketch Primitives

Hand-drawn primitives produce organic, imperfect shapes that simulate pencil, charcoal,
watercolor, and hatching techniques. They use numpy-based value noise for position wobble,
width variation, edge bleeding, and particle scattering.

```lisp
;; Pencil — variable-width line with positional wobble
(pencil x1 y1 x2 y2
  :stroke "#333"        ; Line color (default #000)
  :stroke-width 2       ; Base width (default 2.0)
  :roughness 0.3        ; Wobble amount 0–1 (default 0.3)
  :variance 0.4)        ; Width variation 0–1 (default 0.4)

;; Charcoal — rough stroke with particle scatter
(charcoal x1 y1 x2 y2
  :stroke "#222"        ; Color (default #222)
  :stroke-width 4       ; Base width (default 4.0)
  :roughness 0.5        ; Wobble 0–1 (default 0.5)
  :scatter 0.3)         ; Particle scatter 0–1 (default 0.3)

;; Watercolor rectangle — bleeding edges, translucent layers
(watercolor-rect x y w h
  :fill "#e74c3c"       ; Fill color
  :bleed 0.3            ; Edge bleeding 0–1 (default 0.3)
  :layers 3)            ; Translucent passes (default 3)

;; Watercolor circle
(watercolor-circle cx cy r
  :fill "#3498db"
  :bleed 0.4
  :layers 4)

;; Hatching / cross-hatching fill
(hatch x y w h
  :stroke "#555"        ; Line color (default #333)
  :stroke-width 1       ; Line thickness (default 1.0)
  :density 0.4          ; Line spacing 0–1 (default 0.4)
  :angle 45             ; Hatch angle in degrees (default 45)
  :cross #f)            ; Enable cross-hatching (default #f)

;; Sketchify — apply organic warp to any existing shape
(sketchify shape
  :roughness 0.2)       ; Distortion amount 0–1 (default 0.2)
```

**Composition example:**

```lisp
(canvas 200 200 :bg "#f5f0e8")              ;; warm paper tone
(add (watercolor-circle 60 150 70 :fill "#7f8c8d" :bleed 0.3 :layers 3))  ;; mountain
(add (pencil 100 100 100 160 :stroke "#5d4037" :stroke-width 3 :roughness 0.4 :variance 0.5))  ;; tree trunk
(add (watercolor-circle 100 80 40 :fill "#27ae60" :bleed 0.4 :layers 4))  ;; canopy
(add (hatch 0 160 200 40 :stroke "#8d6e63" :density 0.3 :angle 15 :cross #f))  ;; ground
(render "landscape.png")
```

All sketch shapes accept optional `:seed` for reproducible noise. For a full demo:

```bash
uv run pml examples/handdrawn.pml -o examples_output
```

---

## 6. Semantic Components

### Style System

```lisp
;; Define a custom style
(define-style "neon"
  :outline-width 3.0
  :outline-color "#00ff88"
  :shading "cel"
  :highlight #t)

;; Switch active style
(use-style 'cel)            ; Use built-in cel style
(use-style "neon")          ; Use custom style
```

### Palette

```lisp
;; Define a color set
(define-palette "ocean"
  :primary "#0077be"
  :secondary "#00bfff"
  :accent "#ffd700")

;; Use a palette
(use-palette 'ocean)
```

### Components

| Category | Components |
|----------|------------|
| character | `(character ...)` — full character assembly |
| body | `(body :skin "#fce4c8" :build "slim" :proportions "anime")` |
| head | `(head :shape "oval" :skin "#fce4c8" :ears "normal")` |
| eyes | `(anime-eyes :style "shoujo" :color "#4a90d9" :size 1.0)` |
| hair | `(hair :style "long" :color "#8B4513" :bangs #t)` |
| mouth | `(mouth :style "smile" :size 1.0)` |
| outfit | `(outfit :top 'armor :bottom 'armor :shoes 'boots :detail 'pattern)` |
| items | `(weapon :type 'sword :material 'gold)`, `(potion :type 'health)`, `(chest ...)`, `(generic-item ...)` |
| ui | `(panel ...)`, `(button ...)`, `(health-bar ...)`, `(icon :type 'heart)` |
| scene | `(tile :type 'grass)`, `(decoration :type 'tree)`, `(background :type 'forest')` |

Use `preview_params` from the MCP API or inspect `stdlib/sprites/*.pml` for full parameter specs.

### Full Character Example

```lisp
(sprite-canvas 128 256 :bg "transparent" :anchor 'center-bottom)

(define hero
  (character
    :outfit (outfit :top 'armor :bottom 'armor :shoes 'boots)
    :weapon (weapon :type 'sword :material 'crystal :element 'holy)
    :expression "happy"
    :style 'cel))

(add hero)
(render "examples_output/hero.png")
```

---

## 7. Animation

### Timeline & Animation

```lisp
;; Animate a property over time
(animate target 'property from to duration
  :ease 'linear                ; Easing: linear, sine-in-out, quad-in, quad-out,
                               ;   bounce-out, elastic-out, cubic-in-out, etc.
  :repeat 1                    ; Number of repeats, or 'infinite
  :delay 0)                    ; Delay before start (seconds)

(play)                         ; Start playback
(stop)                         ; Stop and reset
(pause)                        ; Pause
(seek time)                    ; Jump to specific time
(animation-state)              ; Query state → 'idle, 'playing, 'paused

;; Compose animations
(parallel anim1 anim2 ...)     ; Play simultaneously
(sequence anim1 anim2 ...)     ; Play one after another

;; Per-frame callback
(every-frame (lambda () (println "frame!")))
```

### Easing Functions

Built-in easing names (from stdlib/easing.pml): `linear`, `sine-in`, `sine-out`, `sine-in-out`, `quad-in`, `quad-out`, `quad-in-out`, `cubic-in`, `cubic-out`, `cubic-in-out`, `quart-in`, `quart-out`, `quart-in-out`, `quint-in`, `quint-out`, `quint-in-out`, `expo-in`, `expo-out`, `expo-in-out`, `circ-in`, `circ-out`, `circ-in-out`, `bounce-in`, `bounce-out`, `bounce-in-out`, `elastic-in`, `elastic-out`, `elastic-in-out`.

### Animation Example

```lisp
(canvas 200 200 :bg "#1a1a2e")

(define ball (circle 30 30 20 :fill "#e74c3c"))

;; Bouncing ball with easing
(animate ball 'x 30 170 1.0 :ease 'sine-in-out)
(animate ball 'y 30 170 0.4 :ease 'quad-out :delay 0)
(animate ball 'y 170 30 0.4 :ease 'quad-in :delay 0.4)

(play)
(render "examples_output/bounce.gif" :fps 30)
```

---

## 8. Skeleton & IK

### Defining Skeletons

```lisp
(defskeleton name (param-x param-y)
  (joint joint-name
    :pos (dx dy)               ; Position relative to parent
    :length len                ; Bone length
    :angle ang                 ; Initial angle (degrees)
    :min min-angle             ; Constraint: minimum angle (optional)
    :max max-angle))           ; Constraint: maximum angle (optional)

(defskeleton arm (x y)
  (joint shoulder :pos (0 0) :length 40.0 :angle 0.0 :min -120 :max 120)
  (joint elbow   :pos (0 40) :length 35.0 :angle 0.0 :min -150 :max 0)
  (joint wrist   :pos (0 35) :length 0.0  :angle 0.0))
```

### Creating Instances & IK Solving

```lisp
;; Instantiate skeleton at root position
(define arm-inst (instantiate-skeleton arm 50 180))

;; Solve IK: move end effector toward target
(ik-solve instance end-effector tx ty
  :method 'fabrik              ; 'fabrik or 'ccd (default: 'fabrik)
  :iterations 20               ; Max iterations (default: 10)
  :tolerance 0.01              ; Convergence tolerance (default: 0.01)
  :strict #f)                  ; Raise error if not converged (default: #f)

;; Query joint world position → (x y)
(joint-position instance 'elbow)

;; Bind graphic to joint (updates with skeleton animation)
(bind-skin graphic instance joint-name ...)

;; Clear all skin bindings (for reset)
;; (Python: clear_skin_bindings())
```

### IK Example

```lisp
(defskeleton arm (x y)
  (joint shoulder :pos (0 0) :length 40.0 :angle 0.0 :min -120 :max 120)
  (joint elbow   :pos (0 40) :length 35.0 :angle 0.0 :min -150 :max 0)
  (joint wrist   :pos (0 35) :length 0.0  :angle 0.0))

(define arm-inst (instantiate-skeleton arm 50 180))

;; Solve FABRIK IK
(ik-solve arm-inst 'wrist 95 125 :method 'fabrik :iterations 30 :tolerance 0.5)

;; Get updated joint positions
(define wrist-pos (joint-position arm-inst 'wrist))
(println (car wrist-pos) (car (cdr wrist-pos)))  ; x y
```

---

## 9. Module System

Each `.pml` file is a module. Use `provide` to export symbols and `import` to load them.

```lisp
;; my-math.pml
(define (square x) (* x x))
(define (cube x) (* x x x))
(provide square cube)
```

```lisp
;; main.pml
(import "my-math.pml" as math)
(math/square 5)                  ; → 25
```

**Module resolution**: `PML_PATH` env var → `stdlib/` relative to the `pml/` package root.

---

## 10. Standard Library

Located in `stdlib/`:

| File | Contents |
|------|----------|
| `math.pml` | `clamp`, `lerp`, `normalize`, `remap`, `distance`, `sign` |
| `color.pml` | Color utilities |
| `easing.pml` | Easing functions (sine, quad, cubic, bounce, elastic, etc.) |
| `shapes.pml` | `centered-rect`, `diamond`, `arrow` |
| `sprites/character.pml` | Character component API docs |
| `sprites/body.pml` | Body component API docs |
| `sprites/eyes.pml` | Eye component API docs |
| `sprites/hair.pml` | Hair component API docs |
| `sprites/outfit.pml` | Outfit component API docs |
| `sprites/items.pml` | Item component API docs |
| `sprites/ui.pml` | UI widget component API docs |
| `sprites/scene.pml` | Scene element component API docs |
| `sprites/styles.pml` | Style API docs |

---

## 11. CLI Reference

```
usage: pml [-h] [--output-dir OUTPUT_DIR] [--watch] [--gif] [--json] [file]

positional arguments:
  file                  PML source file to execute (omit for REPL)

options:
  -h, --help            Show help
  --output-dir, -o DIR  Output directory for rendered files
  --watch, -w           Watch file for changes and re-run
  --gif                 Force GIF animation output
  --json                Output structured JSON result to stdout
```

### MCP Server

```bash
uv run pml-mcp
```

Connects via stdio transport. Exposes 5 tools:
- `execute_pml` — Run PML code
- `render_sprite` — Render sprite to PNG
- `validate` — Syntax-check without execution
- `list_components` — Discover available components
- `preview_params` — Inspect component parameters

---

## 12. Pipeline Architecture

```
Source (.pml) → Lexer → tokens → Parser → AST → Expander → expanded AST → Evaluator → value → Renderer → PNG/GIF
```

1. **Lexer** (`pml/lexer.py`): Tokenizes source into atoms and lists
2. **Parser** (`pml/parser.py`): Builds AST (nested Python lists)
3. **Expander** (`pml/expander.py`): Macro expansion pass (must run before evaluation)
4. **Evaluator** (`pml/evaluator.py`): Tree-walking interpreter
5. **Render** (`pml/graphics/render.py`): Pillow backend draws to PNG/GIF

Errors propagate as `PMLError` subclasses with structured hints for LLM repair.

---

## 13. Key Idioms & Pitfalls

### Lists vs Function Calls
```lisp
(100 200)    ; ❌ ERROR: tries to call 100 as a function
'(100 200)   ; ✅ data list: (100 200)
(list 100 200)  ; ✅ data list: (100 200)
```

### Accessing List Elements
```lisp
(car '(1 2 3))        ; → 1 (first element)
(cdr '(1 2 3))        ; → (2 3) (rest)
(car (cdr '(1 2 3)))  ; → 2 (second element — no built-in `cadr`)
```

### No `cadr`, `caar`, etc.
PML does **not** define `cadr`, `caar`, `cddr` etc. Use nested `(car (cdr ...))` instead.

### Skeleton IK: Joint Offsets
When using `(defskeleton ... :pos ...)`, the FABRIK solver's `chain_positions` includes joint position offsets. Converting back to angles via `forward_kinematics` may double-apply them. Use `:pos (0 0)` for all joints if positions cause issues.

### Mutability
- `GraphicObject` is frozen — `with_transform()` and `with_param()` return new instances
- `set!` mutates existing variable bindings (searches up the environment chain)
- Animation timeline, canvas, and skin bindings are global singletons

### Reset Between Runs
```python
# Python API: reset environment between independent runs
runtime.reset()

# Clear specific state
from pml.animation.timeline import Timeline
Timeline.reset()

from pml.skeleton import clear_skin_bindings
clear_skin_bindings()
```
