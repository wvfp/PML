; 示例 3：parallel / sequence
; 用 sequence 让方块先移动再变色，用 parallel 让多个对象同时运动。

(set-backend! "skia")
(canvas 500 300 :bg "#F7F9F9")

(define a (circle 60 100 25 :fill "#E74C3C"))
(define b (circle 60 200 25 :fill "#3498DB"))

(add a)
(add b)

; a：先右移，再变绿
(define a-move (animate a 'x 60 440 1.0 :ease 'ease-in-out))
(define a-color (animate a 'fill "#E74C3C" "#2ECC71" 1.0 :ease 'linear))
(sequence a-move a-color)

; b：边右移边变黄
(define b-move (animate b 'x 60 440 2.0 :ease 'ease-out))
(define b-color (animate b 'fill "#3498DB" "#F1C40F" 2.0 :ease 'linear))
(parallel b-move b-color)

(render "03_parallel_and_sequence.gif" :fps 20)
