; 示例 6：模糊 / 锐化 / 卷积
; 展示 blur、sharpen、edge-detect、emboss、convolution。

(set-backend! "skia")

(define comp (make-composition "blur-sharpen" 640 400 :bg "#1A1A2E"))

(define bg (make-layer "bg" (rect 0 0 640 400 :fill "#1A1A2E")))

; 原图
(define original (bitmap-layer "../assets/character.png"
                               :name "original"
                               :x 40 :y 120
                               :width 128 :height 128))

; 高斯模糊
(define blurred (bitmap-layer "../assets/character.png"
                              :name "blurred"
                              :x 200 :y 120
                              :width 128 :height 128
                              :filter (blur :radius 4.0)))

; 锐化
(define sharpened (bitmap-layer "../assets/character.png"
                                :name "sharpened"
                                :x 360 :y 120
                                :width 128 :height 128
                                :filter (sharpen :amount 1.5 :radius 1.0)))

; 边缘检测
(define edges (bitmap-layer "../assets/character.png"
                            :name "edges"
                            :x 40 :y 260
                            :width 128 :height 128
                            :filter (edge-detect :type "sobel")))

; 浮雕
(define embossed (bitmap-layer "../assets/character.png"
                               :name "embossed"
                               :x 200 :y 260
                               :width 128 :height 128
                               :filter (emboss :direction 45.0)))

; 自定义卷积（边缘增强核）
(define convolved (bitmap-layer "../assets/character.png"
                                :name "convolved"
                                :x 360 :y 260
                                :width 128 :height 128
                                :filter (convolution :width 3
                                                    :height 3
                                                    :kernel '(-1 -1 -1
                                                              -1  9 -1
                                                              -1 -1 -1)
                                                    :divisor 1.0
                                                    :offset 0.0)))

; 运动模糊
(define motion-blur (bitmap-layer "../assets/character.png"
                                  :name "motion-blur"
                                  :x 500 :y 120
                                  :width 128 :height 128
                                  :filter (blur :type "motion" :radius 6.0 :angle 30.0)))

(set! comp (composition-add comp bg original blurred sharpened edges embossed convolved motion-blur))
(composition-render comp "06_blur_sharpen_convolution.png")
