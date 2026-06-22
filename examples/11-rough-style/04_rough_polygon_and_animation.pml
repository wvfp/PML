; ═══════════════════════════════════════════════════════════════════════════════
; Rough Polygon & Animation
; ───────────────────────────────────────────────────────────────────────────────
; Demonstrates rough polygons with various fill patterns, plus a simple
; rough animation exporting to GIF.
; ═══════════════════════════════════════════════════════════════════════════════

; ── Part 1: Polygons with rough strokes and fill patterns ────────────────────
(canvas 640 360 :bg "#f5f0eb")

; Triangle (3 points) — rough stroke only
(add (polygon (list (list 60 180) (list 20 30) (list 100 30))
              :fill "#e74c3c" :stroke "#c0392b"
              :stroke-width 2 :roughness 2.5))

; Pentagon — rough stroke + hachure fill
(add (polygon (list (list 220 170) (list 250 40)
                    (list 310 20) (list 340 170)
                    (list 280 190))
              :fill "#3498db" :stroke "#2980b9"
              :stroke-width 2 :roughness 1.5
              :fill-style "hachure"))

; Hexagon — exact stroke + cross-hatch fill (pattern only, stroke stays clean)
(add (polygon (list (list 440 170) (list 480 40)
                    (list 540 40) (list 580 170)
                    (list 540 200) (list 480 200))
              :fill "#2ecc71" :stroke "#27ae60"
              :stroke-width 2 :roughness 0
              :fill-style "cross-hatch"))

; Quadrilateral — rough stroke + zigzag fill
(add (polygon (list (list 130 320) (list 150 260)
                    (list 170 280) (list 110 280))
              :fill "#f39c12" :stroke "#e67e22"
              :stroke-width 2 :roughness 2
              :fill-style "zigzag"))

; Pentagon — rough stroke + dots fill
(add (polygon (list (list 300 320) (list 330 250)
                    (list 380 250) (list 390 320)
                    (list 340 340))
              :fill "#9b59b6" :stroke "#8e44ad"
              :stroke-width 2 :roughness 1.5
              :fill-style "dots"))

; Arrow shape — rough stroke + dashed fill
(add (polygon (list (list 480 320) (list 560 280)
                    (list 640 280) (list 560 240)
                    (list 640 240) (list 560 200)
                    (list 480 200) (list 520 260))
              :fill "#1abc9c" :stroke "#16a085"
              :stroke-width 2 :roughness 2
              :fill-style "dashed"))

(render "rough_polygon.png")

; ── Part 2: Animated rough shapes → GIF ──────────────────────────────────────
; Note: for the animation part, we use a separate frame
(canvas 300 200 :bg "#1a1a2e")

(define ball
  (circle 50 100 40 :fill "#e74c3c" :stroke "#c0392b"
          :stroke-width 2 :roughness 3 :seed 7
          :fill-style "hachure"))

(add ball)

; Animate position from x=50 to x=250 (1.5s, ease-in-out)
(animate ball 'x 50 250 1.5 :ease 'ease-in-out)
(play)

(render "rough_anim.gif" :fps 12)
