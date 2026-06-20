; 项目 4：程序化风景
; 用层叠多边形、正弦形状的山脉和确定性分布的树木/星星生成风景。

(set-backend! "skia")
(canvas 640 360 :bg "#0B1016")

; 夜空渐变层
(add (rect 0 0 640 360 :fill "#0B1016"))
(add (rect 0 0 640 260 :fill "#16222A"))
(add (rect 0 0 640 140 :fill "#3A6073"))

; 月亮
(add (circle 540 70 36 :fill "#FDFBF7" :stroke "#E0E0E0" :stroke-width 2))
(add (circle 520 55 6 :fill "#3A6073" :opacity 0.6))

; 星星（确定性分布）
(define (star x y r)
  (circle x y r :fill "#FFFFFF" :opacity 0.85))

(star 60 50 2)
(star 130 35 1.5)
(star 210 80 2)
(star 300 45 1.5)
(star 420 70 2)
(star 580 100 1.5)
(star 610 40 2)

; 远山
(add (polygon '((0 360) (160 230) (320 290) (480 210) (640 300) (640 360))
              :fill "#141E30" :stroke "#0F1924" :stroke-width 1))

; 中山
(add (polygon '((0 360) (100 270) (240 320) (400 250) (560 310) (640 280) (640 360))
              :fill "#243B55" :stroke "#1A2E42" :stroke-width 1))

; 近山
(add (polygon '((0 360) (80 300) (200 340) (360 290) (520 340) (640 310) (640 360))
              :fill "#2E4A62" :stroke "#243B55" :stroke-width 1))

; 地面
(add (rect 0 330 640 30 :fill "#1B5E20" :stroke "#144A18" :stroke-width 1))

; 树木生成函数
(define (draw-tree x y scale)
  (define trunk-w (* 6 scale))
  (define trunk-h (* 24 scale))
  (define canopy-r (* 16 scale))
  (group (rect (- x (/ trunk-w 2)) (- y trunk-h) trunk-w trunk-h :fill "#4E342E")
         (circle x (- y trunk-h (/ canopy-r 2)) canopy-r :fill "#2E7D32")
         (circle (- x (* 10 scale)) (- y trunk-h (* scale 6)) (* 12 scale) :fill "#388E3C")
         (circle (+ x (* 10 scale)) (- y trunk-h (* scale 6)) (* 12 scale) :fill "#388E3C")))

; 按固定间隔放置树木
(define (tree-row start-x step count y scale)
  (if (> count 0)
      (begin
        (add (draw-tree start-x y scale))
        (tree-row (+ start-x step) step (- count 1) y scale))))

(tree-row 80 140 4 330 1.0)
(tree-row 40 180 3 340 0.7)

(add (text 320 345 "Generative Landscape" :fill "#FFFFFF" :font-size 18 :align 'center))

(render "04_generative_landscape.png")
