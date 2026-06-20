; 示例 1：预定义样式
; 用 cel、pixel、flat 三种预定义 style 绘制同一角色。

(set-backend! "skia")
(canvas 600 300 :bg "#F8F9FA")

; cel：粗描边 + 高光
(define hero-cel
  (character :style 'cel :expression 'happy :scale 1.0))

; pixel：像素风，无抗锯齿
(define hero-pixel
  (character :style 'pixel :expression 'neutral :scale 1.0))

; flat：无描边平涂
(define hero-flat
  (character :style 'flat :expression 'surprised :scale 1.0))

(add (translate-object hero-cel 110 150))
(add (translate-object hero-pixel 300 150))
(add (translate-object hero-flat 490 150))

(add (text 110 30 "cel" :fill "#2C3E50" :font-size 18))
(add (text 300 30 "pixel" :fill "#2C3E50" :font-size 18))
(add (text 490 30 "flat" :fill "#2C3E50" :font-size 18))

(render "01_predefined_styles.png")
