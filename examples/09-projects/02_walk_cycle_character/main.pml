; 项目 2：行走循环角色
; 用动画改变角色 x/y 坐标，模拟两名角色以不同速度行走。

(set-backend! "skia")
(canvas 360 260 :bg "#87CEEB")

(import "body.pml" as b)

; 地面
(add (rect 0 220 360 40 :fill "#8B4513" :stroke "#5D4037" :stroke-width 2))

; 两名不同速度、大小的行人
(define walker-1 (b/make-walker 30 170 0.85 "warm-skin"))
(define walker-2 (b/make-walker 30 205 0.65 "dark-hero"))

(add walker-1)
(add walker-2)

; 水平移动 + 上下颠簸模拟步态
(define w1-move (animate walker-1 'x 30 310 2.0 :ease 'linear))
(define w1-bob  (animate walker-1 'y 170 160 0.5 :ease 'ease-in-out :repeat 4))

(define w2-move (animate walker-2 'x 30 310 2.5 :ease 'linear))
(define w2-bob  (animate walker-2 'y 205 198 0.625 :ease 'ease-in-out :repeat 4))

(parallel w1-move w1-bob w2-move w2-bob)
(play)

(add (text 180 30 "Walk Cycle" :fill "#FFFFFF" :font-size 20 :align 'center))

(render "02_walk_cycle_character.gif" :fps 12)
