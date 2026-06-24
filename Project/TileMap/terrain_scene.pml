; terrain_scene.pml — 5×5 混合地形等距场景

(set-backend! "skia")
(canvas 1700 950)

(import "grass_lib.pml" as grass)
(import "dirt_lib.pml" as dirt)
(import "stone_lib.pml" as stone)
(import "sand_lib.pml" as sand)
(import "snow_lib.pml" as snow)

(define tw 300)
(define th 150)
(define td 54)
(define ox 800)
(define oy 150)

; 地形类型: 0=草地 1=泥土 2=岩石 3=沙地 4=雪地
(define map-data (list
  (list 0 0 0 1 1)
  (list 0 0 1 1 2)
  (list 0 1 1 2 2)
  (list 3 3 1 2 4)
  (list 3 3 3 4 4)))

(define (iso-to-screen col row)
  (list
    (+ ox (* (/ tw 2) (- col row)))
    (+ oy (* (/ th 2) (+ col row)))))

(define (draw-terrain type cx cy)
  (cond
    ((= type 0) (grass/draw-grass-tile cx cy tw th td grass/grass-shader))
    ((= type 1) (dirt/draw-dirt-tile cx cy tw th td dirt/dirt-shader))
    ((= type 2) (stone/draw-stone-tile cx cy tw th td stone/stone-shader))
    ((= type 3) (sand/draw-sand-tile cx cy tw th td sand/sand-shader))
    ((= type 4) (snow/draw-snow-tile cx cy tw th td snow/snow-shader))
    (else (grass/draw-grass-tile cx cy tw th td grass/grass-shader))))

(define (render-row col row)
  (if (< col 5)
      (let* ((type (list-ref (list-ref map-data row) col))
             (pos (iso-to-screen col row)))
        (begin
          (add (draw-terrain type (car pos) (list-ref pos 1)))
          (render-row (+ col 1) row)))))

(define (render-all row)
  (if (< row 5)
      (begin
        (render-row 0 row)
        (render-all (+ row 1)))))

(render-all 0)

(render "terrain_scene.png")
