; iso_dirt_cube.pml — 泥土正方体单瓦片

(set-backend! "skia")
(canvas 512 640)

(import "dirt_cube_lib.pml" as lib)

(define tw 300)
(define th 150)
(define td 150)
(define cx 256)
(define cy 240)

(add (lib/draw-dirt-cube cx cy tw th td))

(render "iso_dirt_cube.png")
