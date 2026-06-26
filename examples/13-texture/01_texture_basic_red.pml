;; PML Texture — 基本示例
(set-backend! "skia")
(canvas 400 400 :bg "#1a1a2e")

(define-texture starry-sky (128 128)
  (group
    (rect 0 0 128 128 :fill "#0f0c29")
    (rect 0 0 128 64  :fill "#302b63" :opacity 0.5)
    (circle 96 32 16 :fill "#ffe066")
    (circle 100 28 14 :fill "#302b63")
    (circle 16 24 1.5 :fill "#ffffff")
    (circle 40 48 1   :fill "#ffffff" :opacity 0.8)
    (circle 64 20 2   :fill "#ffffff")
    (circle 24 64 1.5 :fill "#ffffff")
    (circle 48 80 1   :fill "#ffffff" :opacity 0.7)
    (circle 100 72 2  :fill "#ffd700")
    (circle 8  96 1   :fill "#ffffff" :opacity 0.5)
    (circle 72 100 1.5 :fill "#ffffff" :opacity 0.9)
  ))

(add (texture-map (rect 40 40 320 320 :rx 16) :source starry-sky :mode :harmonic))
(add (text 200 380 "Starry Sky — Harmonic UV" :fill "#ff0000" :font-size 12 :align 'center))

(render "01_texture_basic_red.png")
