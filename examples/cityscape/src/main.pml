;; PML Cityscape v2 — Main Entry Point
;; 4800×1200 ultra-wide sunset city panorama
;; Uses: opacity, grid-of, gradient shorthand, color-alpha, color-blend, post-process

(import "config.pml" as cfg)
(import "sky.pml" as sky)
(import "skyline.pml" as skyline)
(import "buildings.pml" as bld)
(import "street.pml" as street)
(import "details.pml" as det)
(import "vehicles.pml" as veh)

(println "=== Cityscape v2: rendering 4800×1200 ===")

;; ── Layer 1 — Sky background (gradient shorthand, opacity) ────────────
(canvas cfg/CANVAS-W cfg/CANVAS-H :bg "transparent")
(sky/draw-sky)
(sky/draw-clouds)
(sky/draw-sun)
(println "  Layer 1: sky + haze + clouds done")

;; ── Layer 2 — Far skyline silhouettes (spire, dome, antennas) ─────────
(skyline/draw-skyline)
(println "  Layer 2: far skyline with landmarks done")

;; ── Layer 3 — Mid buildings (zoned palettes, glass, grid-of windows) ──
(bld/draw-mid-buildings)
(println "  Layer 3: mid buildings zoned")

;; ── Layer 4 — Near buildings (glass reflections, billboards, rooftops) ─
(bld/draw-near-buildings)
(println "  Layer 4: near buildings detailed")

;; ── Layer 5 — Street (crosswalks, lane markings) ──────────────────────
(street/draw-street)
(println "  Layer 5: street + crosswalks")

;; ── Layer 6 — Foreground details ─────────────────────────────────────
;; Shop fronts at building bases
(det/draw-shop-fronts)
;; Pedestrians on sidewalks
(det/draw-all-pedestrians)
;; Traffic lights at intersections
(det/draw-traffic-lights)
;; Street lamps with glow (opacity)
(det/draw-all-lamps)
;; Trees with varied foliage
(det/draw-all-trees)
(println "  Layer 6: shops + people + lamps + trees")

;; ── Layer 7 — Vehicles (buses, cars, trucks, grid-of windows) ─────────
(veh/draw-all-vehicles)
(println "  Layer 7: vehicles (cars, buses, trucks)")

;; ── Post-process shader chain ─────────────────────────────────────────
;; Subtle glow bloom on bright elements
(post-process 'bloom :radius 3 :threshold 0.85 :strength 0.2)
;; Warm vignette for sunset mood
(post-process 'vignette :strength 0.35)
;; Very subtle film grain
(post-process 'noise :amount 0.03)
(println "  Post-process: bloom + vignette + grain")

;; ── Render ────────────────────────────────────────────────────────────
(render "cityscape.png")
(println "=== Cityscape v2 complete: cityscape.png ===")
