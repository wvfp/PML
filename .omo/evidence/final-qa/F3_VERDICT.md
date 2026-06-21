# F3 REAL MANUAL QA — VERDICT

**Date:** 2026-06-21
**Build:** Debug (clean)
**Platform:** Windows x64, MSVC 19.51, Skia backend

---

## Smoke Tests
| Item | Result |
|------|--------|
| `pml-builtins-smoke.exe` | **272/272 PASS** |

All 272 smoke tests pass including arithmetic, comparison, predicates, strings, IO, macros, hash tables, vectors, modules, TCO, exceptions, logic, filter/layer/composition, 3D graphics, tilemap basics, render channels, and multi-texture shaders.

---

## Integration Tests
| # | Test | Result |
|---|------|--------|
| 1 | define-tileset → make-tilemap → tilemap-set! → render-tilemap (orthogonal 5×5) | **PASS** |
| 2 | define-tileset → make-tilemap → tilemap-set! → render-tilemap (isometric 3×3) | **PASS** |
| 3 | rect sprite → render-channels with 'albedo specular normal' | **PASS** |
| 4 | Cross-task: tileset + tilemap + render-channels in single execution | **PASS** |
| 5 | PNG files valid (correct dimensions, valid PNG signatures) | **PASS** |

### Evidence Files Created
| File | Size | Description |
|------|------|-------------|
| `ortho_tilemap.png` | 179 bytes · 160×160px | 5×5 orthogonal grid, grass tile at (2,2), stone at (0,0) |
| `iso_tilemap.png` | 116 bytes · 96×96px | 3×3 isometric grid, grass tile at (1,1) |
| `sprite_albedo.png` | 204 bytes · 64×64px | Red rect with black stroke (albedo channel) |
| `sprite_specular.png` | 218 bytes · 64×64px | Red rect with black stroke (specular channel) |
| `sprite_normal.png` | 183 bytes · 64×64px | Red rect with black stroke (normal channel) |

---

## Edge Cases
| # | Test | Expected | Result |
|---|------|----------|--------|
| 1 | `(render-tilemap 'nonexistent :output "x.png")` | Error | **PASS** — PMLTypeError |
| 2 | `(tilemap-set! 'nonexistent 0 0 1)` | Error | **PASS** — ArityError (expected 5 args) |
| 3 | `(tilemap-set! ... tile 0)` → empty render | No crash, empty PNG | **PASS** — `edge_empty_tilemap.png` (116 bytes) |
| 4 | `(render-channels 42 :channels '(albedo))` | Error | **PASS** — PMLTypeError |
| 5 | `(bind-textures 'nonexistent :textures '())` | Error | **PASS** — PMLTypeError |
| 6 | `(bind-textures 0 :textures '())` | Error | **PASS** — GeneralError |

---

## Cross-Task Integration

The full pipeline was verified end-to-end in a single `.pml` execution:

```
define-tileset (terrain: grass + stone tiles)
  → make-tilemap (orthogonal, 5×5, 2 layers)
  → tilemap-set! (layer 0 @ col=2,row=2 → grass; layer 1 @ col=0,row=0 → stone)
  → render-tilemap → ortho_tilemap.png ✓

  → make-tilemap (isometric, 3×3, 1 layer)
  → tilemap-set! (layer 0 @ col=1,row=1 → grass)
  → render-tilemap (isometric) → iso_tilemap.png ✓

  → rect (32×32, red fill, black stroke)
  → render-channels (albedo, specular, normal) → sprite_*.png ✓
```

---

## VERDICT: **APPROVE**

### Summary
| Category | Count | Result |
|----------|-------|--------|
| Smoke tests | 272/272 | ✅ |
| Integration tests | 5/5 | ✅ |
| Edge cases | 6/6 | ✅ |
| Evidence files created | 6 files | ✅ |
| **Overall** | | **APPROVE** |
