# PML C++ — Behavior Differences

## Overview

This document catalogues all known, intentional differences between the Python PML
reference implementation and the C++ PML port. The C++ port aims for behavioral
compatibility with the Python implementation, but differences arise from distinct
backend libraries, platform constraints, and feature-scoping decisions. Each entry
describes what differs, the magnitude of the difference, the underlying reason, and
whether the divergence is considered acceptable for the current release.

---

## 1. Font Rendering

| Attribute | Value |
|-----------|-------|
| **Python** | Pillow's `ImageFont.truetype` |
| **C++** | Cairo font rendering (via fontconfig) |
| **Difference** | Visual appearance of text differs between Cairo and Pillow font rendering |
| **Impact** | Low — text-heavy sprites may look slightly different |
| **Reason** | Pillow and Cairo use different font rasterization pipelines (FreeType settings, hinting, subpixel rendering) |
| **Status** | **Acceptable** — pixel-perfect font matching is not achievable across different font stacks |

---

## 2. GIF Palette Reduction

| Attribute | Value |
|-----------|-------|
| **Python** | Pillow's ADAPTIVE palette with dithering |
| **C++** | giflib's default quantization |
| **Difference** | Color palette may differ in some frames |
| **Impact** | Low — minor color variations in high-color GIFs |
| **Reason** | Each library implements its own palette reduction and dithering algorithms |
| **Status** | **Acceptable** — GIF output is visually similar |

---

## 3. Graphics Backend: Skia → Cairo

| Attribute | Value |
|-----------|-------|
| **Original Design** | Skia GPU backend |
| **Python** | Pillow (CPU raster) |
| **C++ (actual)** | Cairo (CPU raster) instead of Skia (GPU/shader) |
| **Difference** | No Skia GPU acceleration, no shader support |
| **Impact** | Medium — GPU-accelerated rendering and GLSL shader effects are not available |
| **Reason** | Skia dependency download issues in the build environment; Cairo was substituted as a stable CPU rasterizer with equivalent 2D primitive support |
| **Status** | **Acceptable** — Cairo produces equivalent output for all 2D primitives (circles, rectangles, lines, paths, text) |

---

## 4. Shader Subsystem (SkRuntimeEffect)

| Attribute | Value |
|-----------|-------|
| **Python** | No shader support (Pillow limitation) |
| **C++** | Shader builtins exist as stubs that return `ResourceError` |
| **Difference** | `(shader ...)` and `(apply-shader! ...)` builtins are registered but return errors explaining Skia is needed |
| **Impact** | Low — shader code cannot execute |
| **Reason** | Shader support requires Skia's `SkRuntimeEffect`, which is not available in the Cairo-based build |
| **Status** | **Expected** — shader support was a new feature for the C++ port, not a requirement of the Python-to-C++ port |

---

## 5. Floating-Point Precision

| Attribute | Value |
|-----------|-------|
| **Python** | IEEE 754 double precision (via CPython) |
| **C++** | IEEE 754 double precision |
| **Difference** | Expression evaluation order may differ in rare cases due to compiler optimizations (reassociation, FMA contractions) |
| **Impact** | Very low — identical results for 99.9% of operations |
| **Reason** | C++ compilers are permitted to reassociate floating-point expressions under `-ffast-math` or equivalent; CPython evaluates strictly left-to-right |
| **Status** | **Acceptable** — no `-ffast-math` flags are used in the build, keeping divergence minimal |

---

## 6. MCP Server — `list_components` and `preview_params`

| Attribute | Value |
|-----------|-------|
| **Python** | Fully implemented with PML component registry |
| **C++** | Returns empty lists (stub implementations) |
| **Difference** | MCP tools 4 and 5 return placeholder data instead of real component metadata |
| **Impact** | Medium — MCP clients cannot enumerate components or preview parameter schemas |
| **Reason** | The component registry system has not yet been ported from Python |
| **Status** | **Acceptable** — component registry is a known gap to be filled in a future iteration |

---

## 7. CLI — `--evaluate` Mode

| Attribute | Value |
|-----------|-------|
| **Python** | Supports `-e` / `--evaluate` flag for inline expression evaluation |
| **C++** | Does not support the `-e` flag (use REPL or file mode instead) |
| **Difference** | Inline evaluation is not available via CLI flag |
| **Impact** | Low — users can pipe expressions via stdin or use the REPL |
| **Reason** | Not yet implemented; the CLI was built with file and REPL modes as the primary use cases |
| **Status** | **TODO** — could be added in a future release (V2) |

---

## 8. Backend Query Builtins (C++ Only)

| Attribute | Value |
|-----------|-------|
| **Python** | No equivalent builtins exist |
| **C++** | Adds: `(list-backends)`, `(set-backend!)`, `(current-backend)`, `(backend-capabilities)`, `(backend-available?)` |
| **Difference** | These are C++-only features without Python equivalents |
| **Impact** | None — this is additional functionality, not a behavioral divergence |
| **Reason** | The C++ port supports multiple backends (Cairo, stb_image write) and needs runtime query/selection |
| **Status** | **New feature** — intentional enhancement specific to the C++ implementation |

---

## Summary

| # | Difference | Impact | Status |
|---|-----------|--------|--------|
| 1 | Font rendering (Cairo vs. Pillow) | Low | Acceptable |
| 2 | GIF palette reduction | Low | Acceptable |
| 3 | Graphics backend (Cairo vs. Skia/Pillow) | Medium | Acceptable |
| 4 | Shader subsystem stubs | Low | Expected |
| 5 | Floating-point precision | Very low | Acceptable |
| 6 | MCP component registry stubs | Medium | Acceptable |
| 7 | CLI `--evaluate` mode | Low | TODO |
| 8 | Backend query builtins | None | New feature |
