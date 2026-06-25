;; PML Texture — Wrap 模式对比
(set-backend! "skia")
(canvas 700 350 :bg "#1a1a2e")

(define-texture tile (32 32)
  (group
    (rect 0 0 32 32 :fill "#2d3436")
    (rect 2 2 12 12 :fill "#e17055" :rx 2)
    (rect 18 18 12 12 :fill "#74b9ff" :rx 2)
    (rect 2 18 12 12 :fill "#ffeaa7" :rx 2)
    (rect 18 2 12 12 :fill "#55efc4" :rx 2)
    (circle 16 16 1.5 :fill "#ffffff")
  ))

;; Clamp — 纹理不重复
(add (with-transform (texture-map (circle 100 120 80) :source tile :mode :planar
                           :wrap-x :clamp :wrap-y :clamp)
       (translate 0 0)))
(add (text 100 220 "Clamp" :fill "#ffffff" :font-size 12 :align 'center))

;; Repeat — 纹理平铺
(add (with-transform (texture-map (circle 330 120 80) :source tile :mode :planar
                           :wrap-x :repeat :wrap-y :repeat)
       (translate 0 0)))
(add (text 330 220 "Repeat" :fill "#ffffff" :font-size 12 :align 'center))

;; Mirror — 纹理镜像
(add (with-transform (texture-map (circle 560 120 80) :source tile :mode :planar
                           :wrap-x :mirror :wrap-y :mirror)
       (translate 0 0)))
(add (text 560 220 "Mirror" :fill "#ffffff" :font-size 12 :align 'center))

(render "03_wrap_modes.png")
