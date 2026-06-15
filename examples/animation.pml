;; PML Animation Demo
;; Demonstrates: property animation, easing, timeline playback, GIF export
;;
;; Run: uv run pml examples/animation.pml
;; Output: examples_output/bounce.gif

(println "=== PML Animation Demo ===")

;; === 1. Bouncing Ball with Easing ===
(println "\n--- 1. Bouncing Ball ---")

(canvas 200 200 :bg "#1a1a2e")

(define ball (circle 30 30 20 :fill "#e74c3c"))

(animate ball 'x 30 170 1.0 :ease 'sine-in-out)
(animate ball 'y 30 170 0.4 :ease 'quad-out :delay 0)
(animate ball 'y 170 30 0.4 :ease 'quad-in :delay 0.4)
(animate ball 'y 30 170 0.35 :ease 'quad-out :delay 0.8)
(animate ball 'y 170 60 0.35 :ease 'quad-in :delay 1.15)

(play)

(render "examples_output/bounce.gif" :fps 30)
(println "  bounce.gif rendered!")


;; === 2. Color Cycling ===
(println "\n--- 2. Color Cycling ---")

(canvas 200 200 :bg "#1a1a2e")

(define color-box (rect 40 40 120 120 :fill "#3498db"))

(animate color-box 'fill "#3498db" "#9b59b6" 1.0 :ease 'sine-in-out)
(animate color-box 'fill "#9b59b6" "#e74c3c" 1.0 :ease 'sine-in-out :delay 1.0)
(animate color-box 'fill "#e74c3c" "#2ecc71" 1.0 :ease 'sine-in-out :delay 2.0)
(animate color-box 'fill "#2ecc71" "#3498db" 1.0 :ease 'sine-in-out :delay 3.0)

(play)

(render "examples_output/color_cycle.gif" :fps 15)
(println "  color_cycle.gif rendered!")


;; === 3. Combined Transform Animation ===
(println "\n--- 3. Combined Transform ---")

(canvas 200 200 :bg "#1a1a2e")

(define logo (circle 100 100 40 :fill "#f1c40f" :stroke "#e67e22" :stroke-width 3))

(animate logo 'x 60 140 0.8 :ease 'sine-in-out)
(animate logo 'x 140 60 0.8 :ease 'sine-in-out :delay 0.8)
(animate logo 'y 60 140 0.5 :ease 'quad-out :delay 0)
(animate logo 'y 140 60 0.5 :ease 'quad-in :delay 0.5)
(animate logo 'y 60 140 0.5 :ease 'quad-out :delay 1.0)
(animate logo 'y 140 60 0.5 :ease 'quad-in :delay 1.5)
(animate logo 'fill "#f1c40f" "#e74c3c" 0.5 :ease 'linear)
(animate logo 'fill "#e74c3c" "#f1c40f" 0.5 :ease 'linear :delay 0.5)

(play)

(render "examples_output/combined.gif" :fps 30)
(println "  combined.gif rendered!")


(println "\n=== All animation demos complete ===")
