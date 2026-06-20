; 示例 2：seek / animation-state
; 创建动画后跳到中间时间点，输出中间帧 GIF。

(set-backend! "skia")
(canvas 400 300 :bg "#0F3460")

(define box
  (rect 50 120 60 60
        :fill "#E94560"
        :stroke "#FFFFFF"
        :stroke-width 2
        :rx 8))

(add box)

(define anim (animate box 'x 50 290 2.0 :ease 'ease-out))

; 跳到 1.0 秒（动画总时长 2.0 秒的一半）
(seek 1.0)
(println "after seek, animation-state = " (animation-state))

; 用 fps 强制按动画方式渲染，得到中间帧
(render "02_seek_and_state.gif" :fps 1)
