; 示例 5：composition 层动画
; 用 composition-animate 对图层的 offset / opacity 做逐帧变化，导出 spritesheet。

(set-backend! "skia")

(define comp (make-composition "animated-comp" 400 300 :bg "#2C3E50"))

(define bg (make-layer "bg"
                       (rect 0 0 400 300 :fill "#34495E")
                       :opacity 1.0))

(define box (make-layer "box"
                        (rect 0 0 80 80 :fill "#E74C3C" :rx 8)
                        :offset '(50 110)
                        :opacity 1.0))

(define circle-layer (make-layer "circle"
                                 (circle 0 0 40 :fill "#F1C40F")
                                 :offset '(270 110)
                                 :opacity 0.5))

(set! comp (composition-add comp bg box circle-layer))

; step-fn 接收 (frame-index total-frames t)，返回一帧的图层或图层列表
(define (step-fn i n t)
  (define x (+ 50 (* 220 (/ i (max 1 (- n 1))))))
  (define opacity (+ 0.3 (* 0.7 (/ i (max 1 (- n 1))))))
  (list
    bg
    (layer-with box :offset (list x 110) :opacity opacity)
    (layer-with circle-layer :offset (list (- 350 x) 110) :opacity (- 1.0 (* 0.5 (/ i (max 1 (- n 1))))))))

(composition-animate comp
                     "05_animated_composition"
                     :frames 12
                     :fps 12
                     :cols 4
                     :step-fn step-fn)
