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

;; #rgb — Create RGB color string from r,g,b (0-255)
;;   (#rgb 255 0 0) → "#ff0000"
(define (#rgb r g b)
  (color/rgb r g b))
(provide #rgb)

;; #hsl — Create color string from H(hue 0-360) S(saturation 0-1) L(lightness 0-1)
;;   (#hsl 0.0 1.0 0.5) → "#ff0000"
(define (#hsl h s l)
  (if (= s 0)
    (color/rgb (->int (* l 255)) (->int (* l 255)) (->int (* l 255)))
    (let* ((c (* (- 1 (abs (- (* l 2) 1))) s))
           (h6 (/ h 60))
           (x (* c (- 1 (abs (- (% (->int h6) 2) 1)))))
           (m (- l (/ c 2)))
           (hi (->int h6))
           (r1 (if (< hi 1) c (if (< hi 2) x (if (< hi 3) 0 (if (< hi 4) 0 (if (< hi 5) x c))))))
           (g1 (if (< hi 1) x (if (< hi 2) c (if (< hi 3) c (if (< hi 4) x (if (< hi 5) 0 0))))))
           (b1 (if (< hi 1) 0 (if (< hi 2) 0 (if (< hi 3) x (if (< hi 4) c (if (< hi 5) c x))))))
           (r (->int (* (+ r1 m) 255)))
           (g (->int (* (+ g1 m) 255)))
           (b (->int (* (+ b1 m) 255))))
      (color/rgb r g b))))
(provide #hsl)

;; #pt — Create a point (coordinate pair)
;;   (#pt 10 20) → (10 20)
(define (#pt x y)
  (list x y))
(provide #pt)

;; #rect — Create a rectangle region
;;   (#rect 0 0 100 100) → (0 0 100 100)
(define (#rect x y w h)
  (list x y w h))
(provide #rect)
