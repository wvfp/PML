; iso_dirt_tile.pml — 单个泥土瓦片 (从 dirt_lib.pml 导入)

(set-backend! "skia")
(canvas 512 512)

(import "dirt_lib.pml" as lib)

(define tw 300)
(define th 150)
(define td 54)
(define cx 256)
(define cy 210)

(add (lib/draw-dirt-tile cx cy tw th td lib/dirt-shader))

(render "iso_dirt_tile.png")
