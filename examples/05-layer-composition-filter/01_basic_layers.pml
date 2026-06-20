; 示例 1：基础图层
; 用 make-layer 创建纯色层、形状层，并调整 opacity、visible、locked。

(set-backend! "skia")

(define comp (make-composition "basic-layers" 400 300 :bg "#ECF0F1"))

; 背景层
(define bg (make-layer "bg"
                       (rect 0 0 400 300 :fill "#F8F9FA")
                       :opacity 1.0))

; 红色方块层，半透
(define red-box (make-layer "red-box"
                            (rect 40 40 120 120 :fill "#E74C3C" :rx 8)
                            :offset '(0 0)
                            :opacity 0.8))

; 蓝色圆形层
(define blue-circle (make-layer "blue-circle"
                                (circle 0 0 60 :fill "#3498DB")
                                :offset '(260 150)
                                :opacity 1.0))

; 隐藏层（visible #f）不会被渲染
(define hidden-layer (make-layer "hidden"
                                 (rect 0 0 400 300 :fill "#000000")
                                 :visible #f))

; 锁定层不影响渲染，仅作标记
(define locked-layer (make-layer "locked"
                                 (rect 300 40 60 60 :fill "#F39C12")
                                 :locked #t))

(set! comp (composition-add comp bg red-box blue-circle hidden-layer locked-layer))
(composition-render comp "01_basic_layers.png")
