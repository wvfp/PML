; 示例 5：色彩滤镜
; 展示 color-adjust、levels、curves、threshold、posterize。

(set-backend! "skia")

(define comp (make-composition "color-filters" 640 400 :bg "#111111"))

(define bg (make-layer "bg" (rect 0 0 640 400 :fill "#111111")))

; 原图
(define original (bitmap-layer "../assets/character.png"
                               :name "original"
                               :x 40 :y 120
                               :width 128 :height 128))

; 高对比高饱和
(define stylized (bitmap-layer "../assets/character.png"
                               :name "stylized"
                               :x 200 :y 120
                               :width 128 :height 128
                               :filter (color-adjust :contrast 1.4 :saturation 1.6)))

; 黑白
(define bw (bitmap-layer "../assets/character.png"
                         :name "bw"
                         :x 360 :y 120
                         :width 128 :height 128
                         :filter (color-adjust :grayscale #t)))

; 反色
(define inverted (bitmap-layer "../assets/character.png"
                               :name "inverted"
                               :x 40 :y 260
                               :width 128 :height 128
                               :filter (color-adjust :invert #t)))

; 阈值
(define thresh (bitmap-layer "../assets/character.png"
                             :name "thresh"
                             :x 200 :y 260
                             :width 128 :height 128
                             :filter (threshold :value 128)))

; 色调分离
(define poster (bitmap-layer "../assets/character.png"
                             :name "poster"
                             :x 360 :y 260
                             :width 128 :height 128
                             :filter (posterize :levels 4)))

; Levels 曲线
(define leveled (bitmap-layer "../assets/character.png"
                              :name "leveled"
                              :x 500 :y 120
                              :width 128 :height 128
                              :filter (levels :in-low 20 :in-high 220 :out-low 10 :out-high 240 :gamma 1.2)))

; Curves
(define curved (bitmap-layer "../assets/character.png"
                             :name "curved"
                             :x 500 :y 260
                             :width 128 :height 128
                             :filter (curves :channel "rgb"
                                             :points '((0 30) (80 80) (160 180) (255 240)))))

(set! comp (composition-add comp bg original stylized bw inverted thresh poster leveled curved))
(composition-render comp "05_color_filters.png")
