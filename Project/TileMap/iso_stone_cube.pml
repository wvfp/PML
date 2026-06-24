; iso_stone_cube.pml — 岩石正方体单瓦片

(set-backend! "skia")
(canvas 512 640)

(import "stone_cube_lib.pml" as lib)

(define tw 300)
(define th 150)
(define td 150)
(define cx 256)
(define cy 240)

(add (lib/draw-stone-cube cx cy tw th td))

(render "iso_stone_cube.png")
