; 示例 2：自定义样式
; 用 define-style 注册新样式，再用 use-style 获取并应用到角色。

(set-backend! "skia")
(canvas 500 300 :bg "#ECF0F1")

; 定义一个粗黑描边、无高亮的卡通样式
(define-style 'cartoon
  :outline-width 4.0
  :outline-color "#1A1A1A"
  :outline-style "solid"
  :shading "cel"
  :shadow #f
  :highlight #f
  :anti-alias #t)

; 定义一个细线框、无填充的线稿样式
(define-style 'line-art
  :outline-width 2.0
  :outline-color "#2C3E50"
  :outline-style "solid"
  :shading "flat"
  :shadow #f
  :highlight #f
  :anti-alias #t)

(define cartoon-hero (character :style (use-style 'cartoon) :expression 'happy))
(define line-hero (character :style (use-style 'line-art) :expression 'neutral))

(add (translate-object cartoon-hero 140 150))
(add (translate-object line-hero 360 150))

(add (text 140 30 "cartoon" :fill "#2C3E50" :font-size 18))
(add (text 360 30 "line-art" :fill "#2C3E50" :font-size 18))

(render "02_define_style.png")
