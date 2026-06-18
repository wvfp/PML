;; PML Hand-Drawn / Sketch Demo
;;
;; Run:  uv run pml examples/handdrawn.pml -o examples_output
;;
;; Demonstrates: pencil, charcoal, watercolor-rect, watercolor-circle,
;;               hatch (cross-hatch), and sketchify modifier.

;; ======================================================================
;; 1. Pencil sketch — variable-width line with wobble
;; ======================================================================
(canvas 200 200 :bg "#f5f0e8")                    ;; warm paper tone
(add (pencil 20 20 180 40 :stroke "#333" :stroke-width 2 :roughness 0.3 :variance 0.5))
(add (pencil 20 60 180 65 :stroke "#555" :stroke-width 1.5 :roughness 0.2 :variance 0.3))
(add (pencil 20 100 180 105 :stroke "#222" :stroke-width 3 :roughness 0.6 :variance 0.7))
(add (text 20 150 "Pencil — wobble + variable width" :fill "#555" :size 12))
(add (pencil 20 170 180 172 :stroke "#888" :stroke-width 0.5 :roughness 0.1 :variance 0.1))
(render "handdrawn-pencil.png")

;; ======================================================================
;; 2. Charcoal — rough stroke with scattered particles
;; ======================================================================
(canvas 200 200 :bg "#f5f0e8")
(add (charcoal 20 30 180 50 :stroke "#222" :stroke-width 6 :roughness 0.5 :scatter 0.4))
(add (charcoal 30 80 170 95 :stroke "#333" :stroke-width 4 :roughness 0.4 :scatter 0.3))
(add (charcoal 40 120 160 130 :stroke "#111" :stroke-width 8 :roughness 0.7 :scatter 0.5))
(add (text 20 170 "Charcoal — rough + particle scatter" :fill "#555" :size 12))
(render "handdrawn-charcoal.png")

;; ======================================================================
;; 3. Watercolor rect — bleeding edges, translucent layers
;; ======================================================================
(canvas 200 200 :bg "#f5f0e8")
(add (watercolor-rect 20 20 70 70 :fill "#e74c3c" :bleed 0.4 :layers 4))
(add (watercolor-rect 110 20 70 70 :fill "#3498db" :bleed 0.3 :layers 3))
(add (watercolor-rect 20 110 70 70 :fill "#2ecc71" :bleed 0.5 :layers 5))
(add (watercolor-rect 110 110 70 70 :fill "#f39c12" :bleed 0.2 :layers 2))
(add (text 20 190 "Watercolor Rect — bleeding edge" :fill "#555" :size 12))
(render "handdrawn-watercolor-rect.png")

;; ======================================================================
;; 4. Watercolor circle — organic rounded shapes
;; ======================================================================
(canvas 200 200 :bg "#f5f0e8")
(add (watercolor-circle 50 50 35 :fill "#9b59b6" :bleed 0.4 :layers 4))
(add (watercolor-circle 140 50 30 :fill "#1abc9c" :bleed 0.3 :layers 3))
(add (watercolor-circle 50 140 25 :fill "#e67e22" :bleed 0.5 :layers 5))
(add (watercolor-circle 140 140 40 :fill "#e74c3c" :bleed 0.3 :layers 3))
(render "handdrawn-watercolor-circle.png")

;; ======================================================================
;; 5. Hatching / Cross-hatching
;; ======================================================================
(canvas 200 200 :bg "#f5f0e8")

;; Single-direction hatch
(add (rect 15 15 70 70 :fill "none" :stroke "#aaa" :stroke-width 1))
(add (hatch 15 15 70 70 :stroke "#555" :density 0.4 :angle 45 :cross #f))

;; Cross-hatch (two directions)
(add (rect 110 15 70 70 :fill "none" :stroke "#aaa" :stroke-width 1))
(add (hatch 110 15 70 70 :stroke "#444" :density 0.6 :angle 30 :cross #t))

;; Dense hatch for dark shading
(add (rect 15 110 70 70 :fill "none" :stroke "#aaa" :stroke-width 1))
(add (hatch 15 110 70 70 :stroke "#222" :density 0.8 :angle 45 :cross #t))

;; Sparse hatch for light shading
(add (rect 110 110 70 70 :fill "none" :stroke "#aaa" :stroke-width 1))
(add (hatch 110 110 70 70 :stroke "#999" :density 0.2 :angle 135 :cross #f))

(add (text 20 195 "Hatch — density / angle / cross" :fill "#555" :size 12))
(render "handdrawn-hatch.png")

;; ======================================================================
;; 6. Composition — combining all sketch techniques
;; ======================================================================
(canvas 200 200 :bg "#f5f0e8")

;; Background wash
(add (watercolor-rect 0 0 200 200 :fill "#f5e6c8" :bleed 0.1 :layers 2))

;; Mountains (watercolor)
(add (watercolor-circle 60 150 70 :fill "#7f8c8d" :bleed 0.3 :layers 3))
(add (watercolor-circle 140 160 50 :fill "#95a5a6" :bleed 0.2 :layers 2))

;; Tree trunk (pencil)
(add (pencil 100 100 100 160 :stroke "#5d4037" :stroke-width 3 :roughness 0.4 :variance 0.5))

;; Tree canopy (watercolor circle)
(add (watercolor-circle 100 80 40 :fill "#27ae60" :bleed 0.4 :layers 4))
(add (watercolor-circle 85 70 25 :fill "#2ecc71" :bleed 0.3 :layers 3))
(add (watercolor-circle 115 65 20 :fill "#1e8449" :bleed 0.3 :layers 3))

;; Ground hatch
(add (hatch 0 160 200 40 :stroke "#8d6e63" :density 0.3 :angle 15 :cross #f))

;; Frame (charcoal)
(add (charcoal 5 5 195 5 :stroke "#333" :stroke-width 4 :roughness 0.5 :scatter 0.2))
(add (charcoal 5 195 195 195 :stroke "#333" :stroke-width 4 :roughness 0.5 :scatter 0.2))
(add (charcoal 5 5 5 195 :stroke "#333" :stroke-width 4 :roughness 0.5 :scatter 0.2))
(add (charcoal 195 5 195 195 :stroke "#333" :stroke-width 4 :roughness 0.5 :scatter 0.2))

(render "handdrawn-composition.png")

;; ======================================================================
;; 7. Sketchify modifier — warp any existing shape
;; ======================================================================
(canvas 200 200 :bg "#f5f0e8")
(add (sketchify (circle 100 80 40 :fill "#e74c3c" :stroke "#c0392b" :stroke-width 2) :roughness 0.3))
(add (sketchify (rect 40 130 120 30 :fill "#3498db" :stroke "#2980b9" :stroke-width 2) :roughness 0.2))
(add (text 20 180 "Sketchify — warp normal shapes" :fill "#555" :size 12))
(render "handdrawn-sketchify.png")

(print "Done! Rendered 7 hand-drawn examples to examples_output/")
