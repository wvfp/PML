; iso_grass_tile.pml — 单个草地瓦片 (从 grass_lib.pml 导入)

(set-backend! "skia")
(canvas 512 512)

(import "grass_lib.pml" as lib)

(define tw 300)
(define th 150)
(define td 54)
(define cx 256)
(define cy 210)

(add (lib/draw-grass-tile cx cy tw th td lib/grass-shader))

(render "iso_grass_tile.png")
