; iso_sand_tile.pml — 单个沙地瓦片 (从 sand_lib.pml 导入)

(set-backend! "skia")
(canvas 512 512)

(import "sand_lib.pml" as lib)

(define tw 300)
(define th 150)
(define td 54)
(define cx 256)
(define cy 210)

(add (lib/draw-sand-tile cx cy tw th td lib/sand-shader))

(render "iso_sand_tile.png")
