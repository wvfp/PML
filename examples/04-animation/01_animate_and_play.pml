; 示例 1：animate / play / stop
; 对圆形的位置与填充色做动画，并导出 GIF。

(set-backend! "skia")
(canvas 400 300 :bg "#1A1A2E")

(define ball
  (circle 50 150 30
          :fill "#FF6B6B"
          :stroke "#FFFFFF"
          :stroke-width 2))

(add ball)

; 位移动画：从左到右
(define move-anim
  (animate ball 'x 50 350 1.5 :ease 'ease-in-out))

; 颜色动画：从红到青
(define color-anim
  (animate ball 'fill "#FF6B6B" "#4ECDC4" 1.5 :ease 'linear))

; 把两个动画组合在一起同时播放
(parallel move-anim color-anim)

(play)
(println "animation-state = " (animation-state))

(render "01_animate_and_play.gif" :fps 24)
