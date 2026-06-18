;; PML Gradient & Bezier Demo
;;
;; Showcases linear/radial gradients, bezier path curves,
;; and combined effects.
;;
;; Usage: uv run pml examples/gradients.pml -o examples_output

;; =====================================================================
;; 1. Linear gradients
;; =====================================================================

(canvas 300 160 :bg "#2c2c2c")

;; Horizontal gradient
(add (rect 20 20 120 50
  :fill "linear-gradient(0deg, #e74c3c, #3498db)"))

;; Vertical gradient (90deg)
(add (rect 160 20 120 50
  :fill "linear-gradient(90deg, #2ecc71, #f1c40f)"))

;; Diagonal gradient (45deg)
(add (rect 20 90 120 50
  :fill "linear-gradient(45deg, #9b59b6, #e67e22, #2ecc71)"))

;; Multi-stop gradient
(add (rect 160 90 120 50
  :fill "linear-gradient(135deg, #e74c3c, #f39c12, #2ecc71, #3498db, #9b59b6)"))

(add (text "Linear Gradients" 10 5 :fill "#aaaaaa" :font-size 10))
(render "gradients_output/linear-gradients.png")

;; =====================================================================
;; 2. Radial gradients
;; =====================================================================

(canvas 300 160 :bg "#2c2c2c")

;; Simple radial
(add (circle 75 80 55
  :fill "radial-gradient(#e74c3c, #2c3e50)"))

;; Radial with 3 stops
(add (circle 225 80 55
  :fill "radial-gradient(#f1c40f, #e67e22, #e74c3c)"))

(add (text "Radial Gradients" 10 5 :fill "#aaaaaa" :font-size 10))
(render "gradients_output/radial-gradients.png")

;; =====================================================================
;; 3. Bezier curve paths
;; =====================================================================

(canvas 320 120 :bg "#2c2c2c")

;; Cubic bezier (C)
(add (path "M 10 60 C 30 10 90 10 110 60" :fill "none" :stroke "#e74c3c" :stroke-width 3))

;; Smooth cubic (S) — continues from previous with reflected control point
(add (path "M 130 60 C 150 10 190 10 210 60 S 250 110 270 60"
  :fill "none" :stroke "#3498db" :stroke-width 3))

(add (text "Bezier Curves: C (cubic) and S (smooth cubic)"
  10 5 :fill "#aaaaaa" :font-size 10))
(render "gradients_output/bezier-curves.png")

;; =====================================================================
;; 4. Quadratic bezier
;; =====================================================================

(canvas 320 120 :bg "#2c2c2c")

;; Quadratic bezier (Q)
(add (path "M 10 60 Q 80 10 150 60" :fill "none" :stroke "#2ecc71" :stroke-width 3))

;; Smooth quadratic (T)
(add (path "M 170 60 Q 200 10 230 60 T 290 60"
  :fill "none" :stroke "#f1c40f" :stroke-width 3))

(add (text "Bezier Curves: Q (quadratic) and T (smooth quad)"
  10 5 :fill "#aaaaaa" :font-size 10))
(render "gradients_output/bezier-quadratic.png")

;; =====================================================================
;; 5. Gradients on shapes
;; =====================================================================

(canvas 320 200 :bg "#2c2c2c")

;; Gradient circle
(add (circle 60 70 50
  :fill "radial-gradient(#f1c40f, #e74c3c, #2c3e50)"))

;; Gradient ellipse
(add (ellipse 180 70 50 35
  :fill "linear-gradient(45deg, #2ecc71, #3498db)"))

;; Gradient polygon (triangle)
(add (polygon '((260 20) (300 90) (220 90))
  :fill "linear-gradient(0deg, #9b59b6, #f1c40f)"))

;; 4. Gradient filled path (heart-ish shape)
(add (path "M 30 130 Q 10 110 15 140 Q 20 170 50 180 Q 80 170 85 140 Q 90 110 70 130 Z"
  :fill "radial-gradient(#e74c3c, #c0392b)" :stroke "#ffffff"))

;; Gradient rect with stroke
(add (rect 180 130 120 50
  :fill "linear-gradient(135deg, #1abc9c, #8e44ad)"
  :stroke "#ecf0f1" :stroke-width 2))

(add (text "Gradients on Circle, Ellipse, Triangle, Path, Rect"
  10 5 :fill "#aaaaaa" :font-size 10))
(render "gradients_output/gradients-on-shapes.png")

;; =====================================================================
;; 6. Gradient + bezier combo — a scenic landscape
;; =====================================================================

(canvas 300 200 :bg "#1a1a2e")

;; Sky gradient
(add (rect 0 0 300 150
  :fill "linear-gradient(90deg, #1a1a2e, #16213e, #0f3460)"))

;; Grass gradient
(add (rect 0 150 300 50
  :fill "linear-gradient(0deg, #27ae60, #1e8449)"))

;; Sun (glowing radial)
(add (circle 250 50 30
  :fill "radial-gradient(#f9e547, #f39c12, rgba(0,0,0,0))"))

;; Mountain path (bezier)
(add (path "M 0 150 L 50 100 L 80 130 L 120 80 L 160 120 L 200 70 L 240 110 L 300 90 L 300 150 Z"
  :fill "linear-gradient(0deg, #2c3e50, #34495e)" :stroke "#1a252f" :stroke-width 1))

;; Clouds (bezier curves)
(add (path "M 40 60 Q 60 40 80 60 Q 100 40 120 60"
  :fill "none" :stroke "rgba(255,255,255,0.3)" :stroke-width 3))
(add (path "M 180 45 Q 200 30 220 45 Q 240 30 260 45"
  :fill "none" :stroke "rgba(255,255,255,0.2)" :stroke-width 2))

(render "gradients_output/landscape-scene.png")

(print "All gradient examples rendered to examples_output/gradients_output/")
