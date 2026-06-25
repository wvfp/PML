;; PML Texture — UV 模式对比
(set-backend! "skia")
(canvas 660 300 :bg "#1a1a2e")

(define-texture checker (64 64)
  (group
    (rect 0  0  32 32 :fill "#e94560")
    (rect 32 0  32 32 :fill "#16213e")
    (rect 0  32 32 32 :fill "#16213e")
    (rect 32 32 32 32 :fill "#e94560")
    (circle 32 32 4 :fill "#ffffff" :opacity 0.8)
  ))

;; Planar UV
(add (with-transform (texture-map (polygon (list (list 100 20) (list 180 60) (list 160 140) (list 40 140) (list 20 60)) :fill "transparent")
                           :source checker :mode :planar)
       (translate 10 30)))

(add (text 60 250 "Planar UV" :fill "#ffffff" :font-size 12 :align 'center))

;; Harmonic UV
(add (with-transform (texture-map (polygon (list (list 100 20) (list 180 60) (list 160 140) (list 40 140) (list 20 60)) :fill "transparent")
                           :source checker :mode :harmonic)
       (translate 230 30)))

(add (text 280 250 "Harmonic UV" :fill "#ffffff" :font-size 12 :align 'center))

;; Explicit UV
(add (with-transform (texture-map (polygon (list (list 100 20) (list 180 60) (list 160 140) (list 40 140) (list 20 60)) :fill "transparent")
                           :source checker :mode :explicit
                           :uv-vertices '(0.0 0.0  1.0 0.0  0.8 1.0  0.5 0.5  0.0 1.0))
       (translate 450 30)))

(add (text 500 250 "Explicit UV" :fill "#ffffff" :font-size 12 :align 'center))

(render "02_uv_modes.png")
