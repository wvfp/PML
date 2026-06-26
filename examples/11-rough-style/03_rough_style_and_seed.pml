; ==========================================================================================================================================================================================================================================═
; Style System & Deterministic Seed
; ------------------------------------------------------------------------------------------------------------------------------------------------------------─
; Demonstrates define-style with roughness params, and seed-based
; determinism (same seed = identical output).
; ==========================================================================================================================================================================================================================================═

; ---- Define reusable styles with roughness ------------------------------------------------------------------------
(define-style 'hand-drawn
  :roughness 3 :bowing 2 :fill-style "hachure")

(define-style 'sketchy
  :roughness 2 :bowing 1.5 :seed 0 :fill-style "cross-hatch")

(define-style 'rough-outline
  :roughness 2.5 :bowing 1 :seed 42 :fill-style "solid")

; ---- Canvas ------------------------------------------------------------------------------------------------------------------------------------─
(canvas 640 320 :bg "#f5f0eb")

; ---- Left panel: style-based rough rendering --------------------------------------------------------------------
(add (circle 80 80 50 :fill "#e74c3c" :stroke "#c0392b"
             :stroke-width 2 :style 'hand-drawn))
(add (rect 20 150 120 80 :fill "#3498db" :stroke "#2980b9"
           :stroke-width 2 :style 'sketchy))
(add (ellipse 80 280 50 30 :fill "#2ecc71" :stroke "#27ae60"
              :stroke-width 2 :style 'rough-outline))

; ---- Middle: per-shape kwargs override style --------------------------------------------------------------------
(add (circle 240 80 50 :fill "#e74c3c" :stroke "#c0392b"
             :stroke-width 2 :style 'hand-drawn
             :roughness 1))  ; override: less rough than style's 3
(add (rect 180 150 120 80 :fill "#3498db" :stroke "#2980b9"
           :stroke-width 2 :style 'sketchy
           :fill-style "dots"))  ; override: dots instead of cross-hatch
(add (ellipse 240 280 50 30 :fill "#2ecc71" :stroke "#27ae60"
              :stroke-width 2 :style 'rough-outline
              :bowing 4))  ; override: stronger bowing

; ---- Right: deterministic seed (same seed = identical rough output) --------------------─
; These two circles are pixel-identical due to same seed = 42
(add (circle 420 80 50 :fill "#e74c3c" :stroke "#c0392b"
             :stroke-width 2 :roughness 3 :seed 42))
(add (circle 500 80 50 :fill "#e74c3c" :stroke "#c0392b"
             :stroke-width 2 :roughness 3 :seed 42))

; These two rectangles are also pixel-identical
(add (rect 370 160 120 80 :fill "#3498db" :stroke "#2980b9"
           :stroke-width 2 :roughness 2 :seed 7
           :fill-style "zigzag"))
(add (rect 500 160 120 80 :fill "#3498db" :stroke "#2980b9"
           :stroke-width 2 :roughness 2 :seed 7
           :fill-style "zigzag"))

(add (text "Same seed = identical output" 400 290 :fill "#666" :font-size 10))

; ---- Labels ------------------------------------------------------------------------------------------------------------------------------------─
(add (text "define-style" 45 18 :fill "#333" :font-size 11))
(add (text "Override kwargs" 210 18 :fill "#333" :font-size 11))
(add (text "Seed determinism" 410 18 :fill "#333" :font-size 11))

(render "rough_style_and_seed.png")

