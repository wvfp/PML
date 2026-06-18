;; PML Cityscape v2 — Street, Sidewalk, Crosswalks
(import "config.pml" as cfg)

(define (draw-sidewalks)
  ;; Upper sidewalk
  (add (rect 0 cfg/SIDEWALK-Y cfg/CANVAS-W cfg/SIDEWALK-H :fill cfg/WALK-CLR))
  ;; Curb edge
  (add (rect 0 (- cfg/SIDEWALK-Y cfg/SIDEWALK-H) cfg/CANVAS-W 3 :fill cfg/CURB-CLR))
  ;; Lower sidewalk
  (add (rect 0 (+ cfg/ROAD-Y cfg/ROAD-H) cfg/CANVAS-W cfg/SIDEWALK-H :fill cfg/WALK-CLR))
  ;; Lower curb
  (add (rect 0 (+ cfg/ROAD-Y cfg/ROAD-H 3) cfg/CANVAS-W 3 :fill cfg/CURB-CLR)))

(define (draw-road)
  (add (rect 0 cfg/ROAD-Y cfg/CANVAS-W cfg/ROAD-H :fill cfg/ROAD-CLR)))

(define (draw-lane-markings)
  ;; 3 lanes → 2 dashed lane lines across 4800px
  (let ((lane1 (+ cfg/ROAD-Y (quotient cfg/ROAD-H 3)))
        (lane2 (+ cfg/ROAD-Y (* 2 (quotient cfg/ROAD-H 3)))))
    (do ((i 0 (+ i 60))) ((> i cfg/CANVAS-W))
      (add (rect i lane1 30 2 :fill cfg/LANE-CLR :opacity 0.7)))
    (do ((i 30 (+ i 60))) ((> i cfg/CANVAS-W))
      (add (rect i lane2 30 2 :fill cfg/LANE-CLR :opacity 0.7)))))

(define (draw-crosswalk cx)
  ;; Zebra crosswalk centered at cx
  (do ((i 0 (+ i cfg/CROSSWALK-GAP))) ((> i cfg/CROSSWALK-W))
    (add (rect (+ cx i) cfg/ROAD-Y 3 cfg/ROAD-H :fill cfg/CROSSWALK-CLR :opacity 0.5))))

(define (draw-all-crosswalks)
  (draw-crosswalk 800)
  (draw-crosswalk 1600)
  (draw-crosswalk 2400)
  (draw-crosswalk 3200)
  (draw-crosswalk 4000))

(define (draw-street)
  (draw-sidewalks)
  (draw-road)
  (draw-lane-markings)
  (draw-all-crosswalks))

(provide draw-street)
