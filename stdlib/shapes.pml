;; PML Standard Library — shapes.pml
;; Reusable shape construction functions
;;
;; Usage: (import "shapes.pml" as shapes)
;;        (shapes/centered-rect 0 0 50 30)

;; Create a centered rectangle
(define (centered-rect cx cy w h)
  (rect :x (- cx (/ w 2)) :y (- cy (/ h 2)) :w w :h h))
(provide centered-rect)

;; Create a diamond shape
(define (diamond cx cy size)
  (polygon :points (list
    cx (- cy size)
    (+ cx size) cy
    cx (+ cy size)
    (- cx size) cy)))
(provide diamond)

;; Create an arrow pointing in a direction
(define (arrow cx cy size dir)
  (polygon :points (list
    cx (- cy size)
    (+ cx (/ size 2)) cy
    (- cx (/ size 2)) cy)))
(provide arrow)
