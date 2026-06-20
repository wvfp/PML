; 示例 3：混合模式
; 多个彩色圆形用不同 BlendMode 叠加。

(set-backend! "skia")

(define comp (make-composition "blend-modes" 600 400 :bg "#FFFFFF"))

(define (sample-circle name color x y mode)
  (make-layer name
              (circle 0 0 50 :fill color)
              :offset (list x y)
              :blend mode))

; 三原色用不同混合模式叠加
(define c1 (sample-circle "red"   "#FF0000" 120 120 "multiply"))
(define c2 (sample-circle "green" "#00FF00" 180 120 "multiply"))
(define c3 (sample-circle "blue"  "#0000FF" 150 170 "multiply"))

(define s1 (sample-circle "r2" "#FF0000" 380 120 "screen"))
(define s2 (sample-circle "g2" "#00FF00" 440 120 "screen"))
(define s3 (sample-circle "b2" "#0000FF" 410 170 "screen"))

(define o1 (sample-circle "r3" "#FF0000" 120 280 "overlay"))
(define o2 (sample-circle "g3" "#00FF00" 180 280 "overlay"))
(define o3 (sample-circle "b3" "#0000FF" 150 330 "overlay"))

(set! comp (composition-add comp c1 c2 c3 s1 s2 s3 o1 o2 o3))
(composition-render comp "03_blend_modes.png")
