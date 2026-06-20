; 示例 2：image 对象
; 用 image 直接加载 PNG 并控制位置、尺寸与局部裁剪。

(set-backend! "skia")
(canvas 400 300 :bg "#87CEEB")

; 完整绘制
(define ground (image "../assets/grass.png" :x 0 :y 236 :width 400 :height 64))
(add ground)

; 缩放绘制
(define tree (image "../assets/tree.png" :x 80 :y 100 :width 128 :height 128))
(add tree)

; 裁剪绘制：只显示 coin 的中心区域并放大
(define coin (image "../assets/coin.png"
                    :x 300 :y 140
                    :width 64 :height 64
                    :crop '(8 8 16 16)))
(add coin)

(add (text 200 40 "Image Object" :fill "#FFFFFF" :font-size 24 :align 'center))

(render "02_image_object.png")
