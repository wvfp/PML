; 示例 3：预定义调色板
; 使用内置 dark-hero 与 warm-skin 调色板绘制角色。

(set-backend! "skia")
(canvas 500 300 :bg "#1A1A2E")

; dark-hero 调色板：深发色、冷色服装
(define hero-dark
  (character :palette "dark-hero"
             :expression 'neutral
             :style 'cel))

; warm-skin 调色板：暖肤色
(define hero-warm
  (character :palette "warm-skin"
             :expression 'happy
             :style 'cel))

(add (translate-object hero-dark 140 150))
(add (translate-object hero-warm 360 150))

(add (text 140 40 "dark-hero" :fill "#FFFFFF" :font-size 18))
(add (text 360 40 "warm-skin" :fill "#FFFFFF" :font-size 18))

(render "03_predefined_palettes.png")
