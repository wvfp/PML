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
| **C++** | Skia font rendering (via `skshaper`) |
| **Difference** | Visual appearance of text differs between Skia and Pillow font rendering |
| **Impact** | Low — text-heavy sprites may look slightly different |
| **Reason** | Pillow and Skia use different font rasterization pipelines (FreeType settings, hinting, subpixel rendering) |
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

## 3. Graphics Backend: Pillow vs Skia

| Attribute | Value |
|-----------|-------|
| **Original Design** | Skia GPU backend |
| **Python** | Pillow (CPU raster) |
| **C++ (actual)** | Skia (CPU/GPU raster with pre-built library) |
| **Difference** | C++ uses Skia; Python uses Pillow. Anti-aliasing, color blending, and gradient sampling may differ slightly. |
| **Impact** | Low — both backends produce equivalent output for all 2D primitives (circles, rectangles, lines, paths, text) |
| **Reason** | C++ port targets Skia for shaders, GPU support, and animation GIF output. Cairo remains available as an optional backend (`PML_BUILD_CAIRO=ON`). |
| **Status** | **Acceptable** — visual differences are minor and documented |

---

## 4. Shader Subsystem (SkRuntimeEffect)

| Attribute | Value |
|-----------|-------|
| **Python** | No shader support (Pillow limitation) |
| **C++** | `shader` and `apply-shader!` compile and apply SkSL via `SkRuntimeEffect` |
| **Difference** | Shaders are a C++-only feature with no Python equivalent |
| **Impact** | None — additional functionality in the C++ port |
| **Reason** | Skia's `SkRuntimeEffect` is available in the Skia-based build; Python has no matching capability |
| **Status** | **New feature** — intentional enhancement specific to the C++ implementation |

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
| **C++** | Fully implemented; returns real component metadata from `ComponentRegistry` |
| **Difference** | None — both implementations expose component names and parameter schemas |
| **Impact** | None |
| **Reason** | The C++ component registry (`src/pml/sprites/registry.cpp`) was ported and populated with the same components as Python |
| **Status** | **Resolved** — no behavioral divergence |

---

## 7. Embedded Standard Library Parity

| Attribute | Value |
|-----------|-------|
| **Python** | Loads `.pml` files directly from `stdlib/` at runtime |
| **C++** | Embeds the same `.pml` files into `embedded_stdlib.h/.cpp` at build time via `tools/embed_stdlib.py` |
| **Difference** | None — the embedded bytes are kept in sync with the Python stdlib |
| **Impact** | None |
| **Reason** | C++ cannot rely on a filesystem `stdlib/` directory in all deployment scenarios; embedding guarantees availability |
| **Status** | **Resolved** — stdlib content is synchronized |

---

## 8. CLI — `--evaluate` Mode

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
| 1 | Font rendering (Skia vs. Pillow) | Low | Acceptable |
| 2 | GIF palette reduction | Low | Acceptable |
| 3 | Graphics backend (Skia vs. Pillow) | Low | Acceptable |
| 4 | Shader subsystem (C++ only) | None | New feature |
| 5 | Floating-point precision | Very low | Acceptable |
| 6 | MCP component registry | None | Resolved |
| 7 | Embedded stdlib parity | None | Resolved |
| 8 | CLI `--evaluate` mode | Low | TODO |
| 9 | Backend query builtins | None | New feature |
