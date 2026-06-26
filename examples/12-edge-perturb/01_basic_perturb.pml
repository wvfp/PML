; ==========================================================================================================================================================================================================================================═
; Edge Perturbation — Basic Usage
; ------------------------------------------------------------------------------------------------------------------------------------------------------------─
; Compare exact polygons (left column) vs edge-perturbed (right column).
; Demonstrates: perturb-polygon + polygon, and direct polygon kwargs.
; ==========================================================================================================================================================================================================================================═

(set-backend! "skia")
(canvas 640 360 :bg "#F5F0EB")

; ---- Helpers ------------------------------------------------------------------------------------------------------------------------------------─
(define (label x y msg)
  (add (text x y msg :fill "#666" :font-size 12)))

; ---- Row 1: Triangle --------------------------------------------------------------------------------------------------------------------
; Exact triangle
(define tri '((60 50) (10 160) (110 160)))
(add (polygon tri :fill "#e74c3c" :stroke "#c0392b" :stroke-width 2))

; Perturbed triangle (via perturb-polygon then polygon)
(define tri-perturbed (perturb-polygon tri :edge-noise 0.2 :edge-subdiv 3 :seed 10))
(add (with-transform (polygon tri-perturbed :fill "#e74c3c" :stroke "#c0392b" :stroke-width 2)
       (translate 160 0)))

(label 40 175 "Exact")
(label 200 175 "Edge-noise=0.2")

; ---- Row 2: Square ------------------------------------------------------------------------------------------------------------------------─
; Exact square
(add (polygon '((20 230) (120 230) (120 330) (20 330))
              :fill "#3498db" :stroke "#2980b9" :stroke-width 2))

; Perturbed square (via polygon kwarg — inline)
(add (with-transform
       (polygon '((20 230) (120 230) (120 330) (20 330))
                :edge-noise 0.15
                :edge-subdiv 4
                :edge-seed 42
                :fill "#3498db" :stroke "#2980b9" :stroke-width 2)
       (translate 160 0)))

(label 40 346 "Exact")
(label 200 346 "Edge-noise=0.15")

; ---- Row 3: Pentagon --------------------------------------------------------------------------------------------------------------------─
; Exact pentagon
(define pent '((420 50) (470 140) (550 120) (560 220) (400 200)))
(add (polygon pent :fill "#2ecc71" :stroke "#27ae60" :stroke-width 2))

; Perturbed pentagon — stronger noise
(define pent-perturbed (perturb-polygon pent :edge-noise 0.25 :edge-subdiv 5 :seed 77))
(add (with-transform (polygon pent-perturbed :fill "#2ecc71" :stroke "#27ae60" :stroke-width 2)
       (translate 160 0)))

(label 400 240 "Exact")
(label 570 240 "Edge-noise=0.25")

(render "01_basic_perturb.png")
