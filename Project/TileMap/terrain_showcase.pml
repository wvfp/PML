; terrain_showcase.pml — 4 种地面瓦片展示 (草地/泥土/岩石/沙地/雪地)

(set-backend! "skia")
(canvas 1200 500)

(import "grass_lib.pml" as grass)
(import "dirt_lib.pml" as dirt)
(import "stone_lib.pml" as stone)
(import "sand_lib.pml" as sand)
(import "snow_lib.pml" as snow)

(define tw 200)
(define th 100)
(define td 40)

; 等距坐标
(define (iso cx cy)
  (list cx cy))

; 5 个瓦片排列
(define positions (list
  (list 150 200)   ; 草地
  (list 370 200)   ; 泥土
  (list 590 200)   ; 岩石
  (list 810 200)   ; 沙地
  (list 1030 200))) ; 雪地

; 渲染瓦片
(define pos1 (car positions))
(add (grass/draw-grass-tile (car pos1) (list-ref pos1 1) tw th td grass/grass-shader))

(define pos2 (list-ref positions 1))
(add (dirt/draw-dirt-tile (car pos2) (list-ref pos2 1) tw th td dirt/dirt-shader))

(define pos3 (list-ref positions 2))
(add (stone/draw-stone-tile (car pos3) (list-ref pos3 1) tw th td stone/stone-shader))

(define pos4 (list-ref positions 3))
(add (sand/draw-sand-tile (car pos4) (list-ref pos4 1) tw th td sand/sand-shader))

(define pos5 (list-ref positions 4))
(add (snow/draw-snow-tile (car pos5) (list-ref pos5 1) tw th td snow/snow-shader))

; 标签
(add (text 150 400 "Grass" :fill "#333" :font-size 18 :align 'center))
(add (text 370 400 "Dirt"  :fill "#333" :font-size 18 :align 'center))
(add (text 590 400 "Stone" :fill "#333" :font-size 18 :align 'center))
(add (text 810 400 "Sand"  :fill "#333" :font-size 18 :align 'center))
(add (text 1030 400 "Snow" :fill "#333" :font-size 18 :align 'center))

(render "terrain_showcase.png")
