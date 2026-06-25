;; PML Texture — 路径纹理
(set-backend! "skia")
(canvas 400 400 :bg "#1a1a2e")

(define-texture rainbow (128 16)
  (group
    (rect 0  0 16 2 :fill "#ff0000")   (rect 16 0 16 2 :fill "#ff7700")
    (rect 32 0 16 2 :fill "#ffff00")   (rect 48 0 16 2 :fill "#00ff00")
    (rect 64 0 16 2 :fill "#0000ff")   (rect 80 0 16 2 :fill "#4b0082")
    (rect 96 0 16 2 :fill "#8b00ff")   (rect 112 0 16 2 :fill "#ff69b4")
  ))

(add (texture-map (rect 40 40 320 160 :rx 8) :source rainbow :wrap-x :repeat :mode :planar))
(add (text 200 380 "Rainbow Texture" :fill "#ffffff" :font-size 14 :align 'center))

(render "04_path_texture.png")
