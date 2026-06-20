; 示例 5：角色组件
; 用 body、head、eyes、mouth、hair、outfit 拼出自定义角色。

(set-backend! "skia")
(canvas 600 350 :bg "#F0F3F4")

(define my-head
  (head :shape 'oval :skin "#F5D0A9" :ears 'normal))

(define my-eyes
  (anime-eyes :style 'shoujo :color "#4A90D9" :size 1.2 :highlight #t))

(define my-mouth
  (mouth :style 'smile :size 1.0))

(define my-hair
  (hair :style 'long :color "#8E44AD" :length 70.0 :bangs #t))

(define my-body
  (body :height 170.0 :build 'slim :skin "#F5D0A9" :proportions 'anime))

(define my-outfit
  (outfit :top 'hoodie :bottom 'pants :shoes 'sneakers
          :color-top "#E74C3C" :color-bottom "#34495E" :detail 'badge))

(define hero
  (character :head my-head
             :body my-body
             :hair my-hair
             :outfit my-outfit
             :expression 'happy
             :style 'cel
             :scale 1.2))

(define villain
  (character :expression 'angry
             :style 'cel
             :palette "dark-hero"
             :scale 1.1))

(add (translate-object hero 180 180))
(add (translate-object villain 420 180))

(add (text 180 60 "Custom hero" :fill "#2C3E50" :font-size 16))
(add (text 420 60 "Default villain" :fill "#2C3E50" :font-size 16))

(render "05_character_component.png")
