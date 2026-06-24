# Draft: Tile Edge Perturbation

## Requirements (confirmed)
- Add edge perturbation to PML polygon shapes so isometric tiles have irregular, natural-looking contours
- Per-edge independent control via vectors/scalars (vector=per-edge, scalar=uniform)
- Shared edges between adjacent tiles must remain straight → world-coordinate based noise ensures matching
- Rounded corners as arc curves (not straight bevel)
- Both Layer 0 (perturb-polygon builtin) and Layer 1 (kwargs on polygon)
- No extrude-tile needed (user builds side faces in PML)
- TDD: GTest + PML smoke tests

## Technical Decisions
- Return type: nested list ((list (list x1 y1) ...)) for direct polygon composition
- Perturbation timing: build-time (in builtin, before GraphicObject creation)
- Noise: self-implemented 2D Perlin noise in polygon_perturb.h/.cpp
- Parameter storage: metadata on GraphicObject, NOT ParamKey enum
- Edge interpolation: Catmull-Rom spline (agreed)
- Negative noise: allowed, clamped to min adjacent edge / 2
- Side face colors: user builds their own, no extrude-tile
- Per-edge indexing: edge i = vertex[i] → vertex[(i+1) % n]

## Scope Boundaries
- IN: CPU 2D Perlin noise, perturb-polygon builtin, polygon kwargs, GTest + smoke tests, API docs
- OUT: extrude-tile (deferred), shader displacement (deferred), tilemap modifications, ParamKey changes

## Test Strategy
- TDD with GTest (polygon_perturb_test.cpp) for numerical verification
- PML smoke tests in builtins_smoke.cpp for integration
- Agent QA: render .pml files and verify PNG outputs

## Open Questions (all resolved)
- Q1-8: All confirmed via question tool
