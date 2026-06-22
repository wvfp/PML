; ═══════════════════════════════════════════════════════════════════════════════
; Rough-Style Stroke Basics
; ───────────────────────────────────────────────────────────────────────────────
; Demonstrates hand-drawn / sketchy strokes on all basic shapes.
; Compare "roughness=0" (exact) vs roughness=2 vs roughness=4.
; ═══════════════════════════════════════════════════════════════════════════════

(canvas 640 320 :bg "#f5f0eb")

; ── Row 1: Circle ─────────────────────────────────────────────────────────────
(add (circle 60 60 40 :fill "#e74c3c" :stroke "#c0392b"
             :stroke-width 2 :roughness 0))
(add (circle 60 180 40 :fill "#e74c3c" :stroke "#c0392b"
             :stroke-width 2 :roughness 2))
(add (circle 60 260 40 :fill "#e74c3c" :stroke "#c0392b"
             :stroke-width 2 :roughness 4 :bowing 3))

; ── Row 2: Rectangle ──────────────────────────────────────────────────────────
(add (rect 160 40 80 80 :fill "#3498db" :stroke "#2980b9"
           :stroke-width 2 :roughness 0))
(add (rect 160 160 80 80 :fill "#3498db" :stroke "#2980b9"
           :stroke-width 2 :roughness 2))
(add (rect 160 260 80 80 :fill "#3498db" :stroke "#2980b9"
           :stroke-width 2 :roughness 4 :bowing 3))

; ── Row 3: Ellipse ────────────────────────────────────────────────────────────
(add (ellipse 340 80 50 30 :fill "#2ecc71" :stroke "#27ae60"
              :stroke-width 2 :roughness 0))
(add (ellipse 340 200 50 30 :fill "#2ecc71" :stroke "#27ae60"
              :stroke-width 2 :roughness 2 :bowing 1.5))
(add (ellipse 340 280 50 30 :fill "#2ecc71" :stroke "#27ae60"
              :stroke-width 2 :roughness 4 :bowing 3))

; ── Row 4: Line ───────────────────────────────────────────────────────────────
(add (line 500 50 580 90 :stroke "#9b59b6" :stroke-width 3 :roughness 0))
(add (line 500 180 580 220 :stroke "#9b59b6" :stroke-width 3 :roughness 2))
(add (line 500 270 580 310 :stroke "#9b59b6" :stroke-width 3 :roughness 4))

; ── Labels ────────────────────────────────────────────────────────────────────
(add (text "roughness=0" 30 30 :fill "#666" :font-size 11))
(add (text "roughness=2" 30 150 :fill "#666" :font-size 11))
(add (text "roughness=4 bowing=3" 30 240 :fill "#666" :font-size 10))

(render "rough_stroke_basics.png")

