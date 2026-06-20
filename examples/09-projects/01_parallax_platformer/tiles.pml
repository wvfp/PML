; tiles.pml — 视差平台场景的共享瓦片绘制函数

(provide draw-ground-tile draw-tree)

(define (draw-ground-tile x y size)
  (group (rect x y size size :fill "#8B4513" :stroke "#5D4037" :stroke-width 2)
         (rect (+ x 4) (+ y 4) (- size 8) 6 :fill "#A0522D")
         (rect (+ x 4) (+ y 16) (- size 8) 4 :fill "#A0522D")))

(define (draw-tree x y scale)
  (define trunk-w (* 8 scale))
  (define trunk-h (* 24 scale))
  (define canopy-r (* 16 scale))
  (group (rect (- x (/ trunk-w 2)) (- y trunk-h) trunk-w trunk-h :fill "#8B4513")
         (circle x (- y trunk-h (/ canopy-r 2)) canopy-r :fill "#228B22")
         (circle (- x (* 10 scale)) (- y trunk-h (* scale 6)) (* 12 scale) :fill "#32CD32")
         (circle (+ x (* 10 scale)) (- y trunk-h (* scale 6)) (* 12 scale) :fill "#32CD32")))
