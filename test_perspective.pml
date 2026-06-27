;; Test: perspective correction (F10)
;; Renders a trapezoid with perspective-correct texture mapping
(canvas 200 200 :bg "#FFFFFF")

;; Create a checkered texture source (50x50 with 4 colored quadrants)
(define-texture checkers (50 50)
  (group
    (rect 0 0 25 25 :fill "#FF0000")
    (rect 25 0 25 25 :fill "#00FF00")
    (rect 0 25 25 25 :fill "#0000FF")
    (rect 25 25 25 25 :fill "#FFFF00")))

;; Draw a trapezoid with perspective-corrected texture
(define mapped (texture-map (polygon '((10 10) (190 10) (170 190) (30 190)))
  :source checkers
  :mode :explicit
  :uv-vertices '((0 0) (1 0) (1 1) (0 1))
  :perspective-correction :true))

(add mapped)

(render "perspective_test.png")
