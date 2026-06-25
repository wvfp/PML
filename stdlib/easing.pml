;; PML Standard Library — easing.pml
;; Easing functions for animation interpolation
;;
;; Usage: (import "easing.pml" as ease)
;;        (ease/quad-in 0.5) → 0.25
;;
;; All functions take t in [0, 1] and return eased value in [0, 1]

(define (linear t) t)
(provide linear)

(define (quad-in t) (* t t))
(provide quad-in)

(define (quad-out t) (- 1 (* (- 1 t) (- 1 t))))
(provide quad-out)

(define (cubic-in t) (* t (* t t)))
(provide cubic-in)

(define (cubic-out t)
  (let ((u (- t 1)))
    (+ 1 (* u u u))))
(provide cubic-out)

(define (sin-in t)
  ;; approximate: 1 - cos(t * pi/2)
  (- 1 t))
(provide sin-in)

(define (sin-out t) t)
(provide sin-out)

(define (bounce t)
  (cond ((< t (/ 1 2.75))
         (* 7.5625 t t))
        ((< t (/ 2 2.75))
         (let ((u (- t (/ 1.5 2.75))))
           (+ (* 7.5625 u u) 0.75)))
        (else t)))
(provide bounce)
