; Example 9: Apply professional filters to a bitmap layer.

(set-backend! "skia")

(define comp (make-composition "filtered" 512 512 :bg "#111111"))

; Base photo layer (use the generated character as a "photo" subject)
(define original (bitmap-layer "../assets/character.png"
                               :name "original"
                               :x 192 :y 192
                               :width 128 :height 128))

; Same image with a dramatic filter chain
(define stylized (bitmap-layer "../assets/character.png"
                               :name "stylized"
                               :x 352 :y 192
                               :width 128 :height 128
                               :filter (filter-chain
                                         (color-adjust :contrast 1.4 :saturation 1.5)
                                         (sharpen :amount 1.5 :radius 1.0))))

; Black & white portrait
(define bw (bitmap-layer "../assets/character.png"
                         :name "bw"
                         :x 32 :y 192
                         :width 128 :height 128
                         :filter (color-adjust :grayscale #t :contrast 1.2)))

; Dreamy blur with outer glow
(define glow (bitmap-layer "../assets/character.png"
                           :name "glow"
                           :x 192 :y 352
                           :width 128 :height 128
                           :filter (filter-chain
                                     (blur :radius 2.0)
                                     (outer-glow :blur 12 :color "#00FFFF80"))))

; Title bar at the top
(define title (make-layer "title"
                          (rect 0 0 512 48 :fill "#222222")
                          :opacity 0.9))

(set! comp (composition-add comp bw original stylized glow title))
(composition-render comp "09_photo_filters.png")
