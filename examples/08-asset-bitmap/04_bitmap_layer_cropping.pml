; 示例 4：Bitmap Layer 裁剪
; 用 bitmap-layer 的 :crop 参数做局部放大/九宫格展示。

(set-backend! "skia")

(define comp (make-composition "bitmap-crop" 400 240 :bg "#2C3E50"))

; 完整角色 layer
(define full (bitmap-layer "../assets/character.png"
                           :name "full"
                           :x 40 :y 40
                           :width 64 :height 64))

; 只显示头部区域并放大
(define head (bitmap-layer "../assets/character.png"
                           :name "head-zoom"
                           :x 180 :y 40
                           :width 80 :height 80
                           :crop '(16 8 32 28)))

; 只显示身体区域
(define body (bitmap-layer "../assets/character.png"
                           :name "body"
                           :x 300 :y 60
                           :width 48 :height 48
                           :crop '(16 32 32 32)))

(set! comp (composition-add comp full head body))
(composition-render comp "04_bitmap_layer_cropping.png")
