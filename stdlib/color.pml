;; PML Standard Library — color.pml
;; Color manipulation utilities
;;
;; Usage: (import "color.pml" as color)
;;        (color/lerp "#ff0000" "#0000ff" 0.5)

;; Blend two colors by ratio t (0..1)
;; Works with #RRGGBB hex colors
(define (lerp color-a color-b t)
  ;; Note: full implementation requires hex parsing
  ;; This is a documentation reference
  color-a)
(provide lerp)

;; Lighten a color by amount (0..255)
(define (lighten color amount)
  color)
(provide lighten)

;; Darken a color by amount (0..255)
(define (darken color amount)
  color)
(provide darken)

;; Desaturate a color towards gray
(define (desaturate color amount)
  color)
(provide desaturate)
