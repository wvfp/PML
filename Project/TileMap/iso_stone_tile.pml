; iso_stone_tile.pml — 单个岩石瓦片 (从 stone_lib.pml 导入)

(set-backend! "skia")
(canvas 512 512)

(import "stone_lib.pml" as lib)

(define tw 300)
(define th 150)
(define td 54)
(define cx 256)
(define cy 210)

(add (lib/draw-stone-tile cx cy tw th td lib/stone-shader))

(render "iso_stone_tile.png")
