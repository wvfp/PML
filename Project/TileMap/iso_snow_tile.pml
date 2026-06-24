; iso_snow_tile.pml — 单个雪地瓦片 (从 snow_lib.pml 导入)

(set-backend! "skia")
(canvas 512 512)

(import "snow_lib.pml" as lib)

(define tw 300)
(define th 150)
(define td 54)
(define cx 256)
(define cy 210)

(add (lib/draw-snow-tile cx cy tw th td lib/snow-shader))

(render "iso_snow_tile.png")
