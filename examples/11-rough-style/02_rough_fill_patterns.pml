; ==========================================================================================================================================================================================================================================═
; Rough-Style Fill Patterns
; ------------------------------------------------------------------------------------------------------------------------------------------------------------─
; Demonstrates the 6 fill patterns: solid, hachure, zigzag, cross-hatch,
; dots, dashed.  Shapes on row 1 use exact strokes (roughness=0) to focus
; on fill patterns.  Row 2 combines rough strokes + pattern fills.
; ==========================================================================================================================================================================================================================================═

(canvas 640 480 :bg "#f5f0eb")

; ---- Row 1: Circles with fill pattern (exact stroke) ----------------------------------------------------
(add (circle 40 50 35 :fill "#e74c3c" :stroke "#333"
             :stroke-width 1.5 :roughness 0 :fill-style "solid"))
(add (text "solid" 22 100 :fill "#333" :font-size 9))

(add (circle 140 50 35 :fill "#3498db" :stroke "#333"
             :stroke-width 1.5 :roughness 0 :fill-style "hachure"))
(add (text "hachure" 110 100 :fill "#333" :font-size 9))

(add (circle 240 50 35 :fill "#2ecc71" :stroke "#333"
             :stroke-width 1.5 :roughness 0 :fill-style "zigzag"))
(add (text "zigzag" 214 100 :fill "#333" :font-size 9))

(add (circle 340 50 35 :fill "#f39c12" :stroke "#333"
             :stroke-width 1.5 :roughness 0 :fill-style "cross-hatch"))
(add (text "cross-hatch" 301 100 :fill "#333" :font-size 9))

(add (circle 440 50 35 :fill "#9b59b6" :stroke "#333"
             :stroke-width 1.5 :roughness 0 :fill-style "dots"))
(add (text "dots" 421 100 :fill "#333" :font-size 9))

(add (circle 540 50 35 :fill "#1abc9c" :stroke "#333"
             :stroke-width 1.5 :roughness 0 :fill-style "dashed"))
(add (text "dashed" 518 100 :fill "#333" :font-size 9))

; ---- Row 2: Rectangles with combined rough stroke + pattern fill ----------------------------
(add (rect 15 130 50 50 :fill "#e74c3c" :stroke "#333"
           :stroke-width 1.5 :roughness 2 :fill-style "solid"))
(add (rect 115 130 50 50 :fill "#3498db" :stroke "#333"
           :stroke-width 1.5 :roughness 2 :fill-style "hachure"))
(add (rect 215 130 50 50 :fill "#2ecc71" :stroke "#333"
           :stroke-width 1.5 :roughness 2 :fill-style "zigzag"))
(add (rect 315 130 50 50 :fill "#f39c12" :stroke "#333"
           :stroke-width 1.5 :roughness 2 :fill-style "cross-hatch"))
(add (rect 415 130 50 50 :fill "#9b59b6" :stroke "#333"
           :stroke-width 1.5 :roughness 2 :fill-style "dots"))
(add (rect 515 130 50 50 :fill "#1abc9c" :stroke "#333"
           :stroke-width 1.5 :roughness 2 :fill-style "dashed"))

; ---- Row 3: Large area showcase --------------------------------------------------------------------------------------------─
(add (text "Hachure fill — large area" 30 240 :fill "#555" :font-size 12))
(add (rect 30 260 260 160 :fill "#e74c3c" :stroke "#c0392b"
           :stroke-width 1 :roughness 0 :fill-style "hachure"))

(add (text "Cross-hatch fill — large area" 330 240 :fill "#555" :font-size 12))
(add (rect 330 260 260 160 :fill "#3498db" :stroke "#2980b9"
           :stroke-width 1 :roughness 0 :fill-style "cross-hatch"))

; ---- Labels ------------------------------------------------------------------------------------------------------------------------------------─
(add (text "Exact stroke + fill pattern" 200 15 :fill "#333" :font-size 11))
(add (text "Rough stroke + fill pattern" 200 115 :fill "#333" :font-size 11))

(render "rough_fill_patterns.png")
