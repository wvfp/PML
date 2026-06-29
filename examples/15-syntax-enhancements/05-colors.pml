;; ============================================================
;; 05-colors.pml — #rgb #pt #rect from stdlib/color.pml
;;
;; Syntax: (#rgb r g b)    → "#rrggbb" color string
;;         (#hsl h s l)    → "#rrggbb" (HSL→RGB conversion)
;;         (#pt x y)       → (x y) list
;;         (#rect x y w h) → (x y w h) list
;; All defined in stdlib/color.pml, loaded automatically.
;; ============================================================
(set-backend! "skia")
(canvas 420 320 :bg "#1a1a2e")

;; --- #rgb: create hex color from r,g,b (0-255) ---
(define hot-pink   (#rgb 255 69 96))
(define mint-green (#rgb 11 232 129))
(define sky-blue   (#rgb 116 185 255))
(define gold       (#rgb 253 203 110))

;; --- #hsl: create color from hue/sat/light ---
(define violet (#hsl 270 0.8 0.6))
(define coral  (#hsl 10 0.9 0.7))

;; --- #pt: create coordinate pairs ---
(define pts (list (#pt 60 80) (#pt 140 70) (#pt 220 90)
                  (#pt 80 200) (#pt 160 220) (#pt 240 200)
                  (#pt 320 80) (#pt 360 200)))
(println "points:" pts)

;; --- #rect: create rect tuples ---
(define border (#rect 15 15 390 290))

;; Draw border (use get to destructure #rect)
(add (rect (get border 0) (get border 1)
           (get border 2) (get border 3)
           :fill "none" :stroke gold :stroke-width 2))

;; Draw circles at each #pt position (manually index with range)
(for-each (lambda (i)
            (let* ((p (get pts i))
                   (x (get p 0)) (y (get p 1)))
              (add (circle x y 18 :fill
                (cond ((< i 3) hot-pink)
                      ((< i 6) mint-green)
                      (#t sky-blue))
                :opacity 0.85))))
          (range 0 8))

;; Big center circle using #hsl color
(add (circle 210 150 50 :fill violet :opacity 0.7))

;; Swatch boxes showing #hsl colors
(add (rect 310 20 90 30 :fill coral :rx 4))
(add (rect 310 270 90 30 :fill violet :rx 4))

(render "05-colors.png")
