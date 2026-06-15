;; PML Skeleton & IK Demo
;; Demonstrates: defskeleton, instantiate-skeleton, ik-solve, joint-position
;;
;; Run: uv run pml examples/skeleton.pml
;; Output: examples_output/skeleton_*.png

(println "=== PML Skeleton & IK Demo ===")

;; === 1. Define Skeleton Templates ===
(println "\n--- 1. Define Skeleton Templates ---")

;; A simple 2-joint arm
(defskeleton arm (x y)
  (joint shoulder :pos (0 0) :length 40.0 :angle 0.0 :min -120 :max 120)
  (joint elbow :pos (0 40) :length 35.0 :angle 0.0 :min -150 :max 0)
  (joint wrist :pos (0 35) :length 0.0 :angle 0.0))

(println "  arm skeleton template defined")

;; A 3-joint leg
(defskeleton leg (x y)
  (joint hip :pos (0 0) :length 35.0 :angle 0.0 :min -60 :max 60)
  (joint knee :pos (0 35) :length 35.0 :angle 0.0 :min 0 :max 150)
  (joint ankle :pos (0 35) :length 0.0 :angle 0.0))

(println "  leg skeleton template defined")


;; === 2. Visualize Skeleton with IK ===
(println "\n--- 2. Arm IK Solved (FABRIK) ---")

(canvas 250 250 :bg "#1a1a2e")

(define arm-inst (instantiate-skeleton arm 50 180))

;; Draw bones as lines and joints as circles
(define shoulder-pos (joint-position arm-inst 'shoulder))
(car shoulder-pos)
(car (cdr shoulder-pos))
(add (circle (car shoulder-pos) (car (cdr shoulder-pos)) 5 :fill "#e74c3c"))

(define elbow-pos (joint-position arm-inst 'elbow))
(add (circle (car elbow-pos) (car (cdr elbow-pos)) 4 :fill "#f39c12"))

(define wrist-pos (joint-position arm-inst 'wrist))
(add (circle (car wrist-pos) (car (cdr wrist-pos)) 3 :fill "#2ecc71"))

;; Draw bones
(add (line (car shoulder-pos) (car (cdr shoulder-pos))
           (car elbow-pos) (car (cdr elbow-pos))
           :stroke "#ffffff" :stroke-width 3))
(add (line (car elbow-pos) (car (cdr elbow-pos))
           (car wrist-pos) (car (cdr wrist-pos))
           :stroke "#ffffff" :stroke-width 3))

;; Target position marker (within reach of the 75-unit arm)
(add (circle 95 125 6 :fill "#e74c3c" :stroke "#ffffff" :stroke-width 2))

;; Solve IK to reach target
(define ik-result (ik-solve arm-inst 'wrist 95 125 :method 'fabrik :iterations 30 :tolerance 0.5))
(println "  IK converged:" ik-result)

;; Redraw after IK
(canvas 250 250 :bg "#1a1a2e")

(define shoulder-pos2 (joint-position arm-inst 'shoulder))
(define elbow-pos2 (joint-position arm-inst 'elbow))
(define wrist-pos2 (joint-position arm-inst 'wrist))

(add (circle (car shoulder-pos2) (car (cdr shoulder-pos2)) 5 :fill "#e74c3c"))
(add (circle (car elbow-pos2) (car (cdr elbow-pos2)) 4 :fill "#f39c12"))
(add (circle (car wrist-pos2) (car (cdr wrist-pos2)) 3 :fill "#2ecc71"))

(add (line (car shoulder-pos2) (car (cdr shoulder-pos2))
           (car elbow-pos2) (car (cdr elbow-pos2))
           :stroke "#ffffff" :stroke-width 3))
(add (line (car elbow-pos2) (car (cdr elbow-pos2))
           (car wrist-pos2) (car (cdr wrist-pos2))
           :stroke "#ffffff" :stroke-width 3))
;; Target position (green = reached)
(add (circle 95 125 6 :fill "#2ecc71" :stroke "#ffffff" :stroke-width 2))

(render "examples_output/arm_ik.png")
(println "  arm_ik.png rendered!")


;; === 3. CCD IK Method ===
(println "\n--- 3. Arm IK Solved (CCD) ---")

(define arm-inst2 (instantiate-skeleton arm 50 180))

;; Draw initial pose
(canvas 250 250 :bg "#1a1a2e")

(define s-pos (joint-position arm-inst2 'shoulder))
(define e-pos (joint-position arm-inst2 'elbow))
(define w-pos (joint-position arm-inst2 'wrist))

(add (circle (car s-pos) (car (cdr s-pos)) 5 :fill "#e74c3c"))
(add (circle (car e-pos) (car (cdr e-pos)) 4 :fill "#f39c12"))
(add (circle (car w-pos) (car (cdr w-pos)) 3 :fill "#2ecc71"))

(add (line (car s-pos) (car (cdr s-pos)) (car e-pos) (car (cdr e-pos))
           :stroke "#ffffff" :stroke-width 3))
(add (line (car e-pos) (car (cdr e-pos)) (car w-pos) (car (cdr w-pos))
           :stroke "#ffffff" :stroke-width 3))

(add (circle 110 130 6 :fill "#e74c3c" :stroke "#ffffff" :stroke-width 2))

(define ik-result2 (ik-solve arm-inst2 'wrist 110 130 :method 'ccd :iterations 50 :tolerance 0.5))
(println "  CCD IK converged:" ik-result2)

;; Redraw solved pose
(canvas 250 250 :bg "#1a1a2e")

(define s-pos2 (joint-position arm-inst2 'shoulder))
(define e-pos2 (joint-position arm-inst2 'elbow))
(define w-pos2 (joint-position arm-inst2 'wrist))

(add (circle (car s-pos2) (car (cdr s-pos2)) 5 :fill "#e74c3c"))
(add (circle (car e-pos2) (car (cdr e-pos2)) 4 :fill "#f39c12"))
(add (circle (car w-pos2) (car (cdr w-pos2)) 3 :fill "#2ecc71"))

(add (line (car s-pos2) (car (cdr s-pos2)) (car e-pos2) (car (cdr e-pos2))
           :stroke "#ffffff" :stroke-width 3))
(add (line (car e-pos2) (car (cdr e-pos2)) (car w-pos2) (car (cdr w-pos2))
           :stroke "#ffffff" :stroke-width 3))

(add (circle 110 130 6 :fill "#2ecc71" :stroke "#ffffff" :stroke-width 2))

(render "examples_output/arm_ccd.png")
(println "  arm_ccd.png rendered!")


(println "\n=== All skeleton demos complete ===")
