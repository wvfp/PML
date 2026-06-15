;; PML Standard Library — math.pml
;; Math utility functions for sprite generation
;;
;; Usage: (import "math.pml" as math)
;;        (math/clamp 15 0 10)  → 10
;;        (math/lerp 0 100 0.5) → 50

;; clamp value to [min-val, max-val]
(define (clamp value min-val max-val)
  (if (< value min-val) min-val
      (if (> value max-val) max-val
          value)))
(provide clamp)

;; linear interpolation between a and b by t (0..1)
(define (lerp a b t)
  (+ a (* (- b a) t)))
(provide lerp)

;; normalize value from [min-val, max-val] to [0, 1]
(define (normalize value min-val max-val)
  (if (= max-val min-val) 0
      (/ (- value min-val) (- max-val min-val))))
(provide normalize)

;; remap value from one range to another
(define (remap value from-min from-max to-min to-max)
  (let ((t (normalize value from-min from-max)))
    (lerp to-min to-max t)))
(provide remap)

;; distance between two 2D points
(define (distance x1 y1 x2 y2)
  (let ((dx (- x2 x1))
        (dy (- y2 y1)))
    ;; approximate sqrt with Newton's method
    (let ((dist-sq (+ (* dx dx) (* dy dy))))
      (if (= dist-sq 0) 0
          ;; simple approximation
          dist-sq))))
(provide distance)

;; sign of a number: -1, 0, or 1
(define (sign x)
  (cond ((> x 0) 1)
        ((< x 0) -1)
        (else 0)))
(provide sign)
