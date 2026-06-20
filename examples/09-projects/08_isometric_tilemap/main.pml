; main.pml - 2.5D 等距 tilemap 大地图（高分辨率版）

(import "isometric.pml" as iso)
(import "decorations.pml" as deco)
(import "map_data.pml" as data)

(set-backend! "skia")
(canvas 5120 3000 :bg "#000000")

; ═══════════════════════════════════════════════════════════════════════════════
; 配置参数
; ═══════════════════════════════════════════════════════════════════════════════

(define tile-w 256)
(define tile-h 128)
(define tile-depth 40)
(define stroke "#3E3A32")
(define stroke-w 2)
(define origin-x 2560)
(define origin-y 400)

; ═══════════════════════════════════════════════════════════════════════════════
; 标题
; ═══════════════════════════════════════════════════════════════════════════════

(add (text 2560 60 "Stardew-style Isometric Tilemap - 20x20" :fill "#E8E8E8" :font-size 48 :align 'center))
(add (text 2560 120 "Grass | Water | Sand | Path | Stone | Dirt" :fill "#B0B0B0" :font-size 28 :align 'center))

; ═══════════════════════════════════════════════════════════════════════════════
; 渲染瓦片地图
; ═══════════════════════════════════════════════════════════════════════════════

(iso/render-tilemap data/map-grid tile-w tile-h tile-depth origin-x origin-y stroke stroke-w)

; ═══════════════════════════════════════════════════════════════════════════════
; 渲染装饰物
; ═══════════════════════════════════════════════════════════════════════════════

(define (place-decoration dec tile-w tile-h origin-x origin-y)
  (define type (list-ref dec 0))
  (define col (list-ref dec 1))
  (define row (list-ref dec 2))
  (define pos (iso/iso-to-screen col row tile-w tile-h origin-x origin-y))
  (define cx (car pos))
  (define cy (list-ref pos 1))
  (cond
    ((eq? type 'tree)
     (if (> (length dec) 3)
         (add (deco/draw-tree-variant cx cy (list-ref dec 3)))
         (add (deco/draw-tree cx cy))))
    ((eq? type 'rock) (add (deco/draw-rock cx cy)))
    ((eq? type 'flower) (add (deco/draw-flower cx cy (list-ref dec 3))))
    ((eq? type 'mushroom) (add (deco/draw-mushroom cx cy)))
    ((eq? type 'bush) (add (deco/draw-bush cx cy)))
    ((eq? type 'crate) (add (deco/draw-crate cx cy)))
    ((eq? type 'fence) (add (deco/draw-fence cx cy)))))

(define (place-all-decorations decs tile-w tile-h origin-x origin-y)
  (if (> (length decs) 0)
      (begin
        (place-decoration (car decs) tile-w tile-h origin-x origin-y)
        (place-all-decorations (cdr decs) tile-w tile-h origin-x origin-y))))

(place-all-decorations data/decorations-list tile-w tile-h origin-x origin-y)

; ═══════════════════════════════════════════════════════════════════════════════
; 图例
; ═══════════════════════════════════════════════════════════════════════════════

(define legend-x 120)
(define legend-y 2700)

(add (text legend-x legend-y "Legend:" :fill "#E8E8E8" :font-size 28))

(add (group
  (rect legend-x (+ legend-y 44) 32 32 :fill "#6B8E41" :stroke "#3A3A3A" :stroke-width 2)
  (text (+ legend-x 44) (+ legend-y 70) "Grass (0)" :fill "#B0B0B0" :font-size 22)))
(add (group
  (rect (+ legend-x 200) (+ legend-y 44) 32 32 :fill "#4A90A4" :stroke "#3A3A3A" :stroke-width 2)
  (text (+ legend-x 244) (+ legend-y 70) "Water (1)" :fill "#B0B0B0" :font-size 22)))
(add (group
  (rect (+ legend-x 400) (+ legend-y 44) 32 32 :fill "#D4C298" :stroke "#3A3A3A" :stroke-width 2)
  (text (+ legend-x 444) (+ legend-y 70) "Sand (2)" :fill "#B0B0B0" :font-size 22)))
(add (group
  (rect legend-x (+ legend-y 88) 32 32 :fill "#BFA77A" :stroke "#3A3A3A" :stroke-width 2)
  (text (+ legend-x 44) (+ legend-y 114) "Path (3)" :fill "#B0B0B0" :font-size 22)))
(add (group
  (rect (+ legend-x 200) (+ legend-y 88) 32 32 :fill "#7D858C" :stroke "#3A3A3A" :stroke-width 2)
  (text (+ legend-x 244) (+ legend-y 114) "Stone (4)" :fill "#B0B0B0" :font-size 22)))
(add (group
  (rect (+ legend-x 400) (+ legend-y 88) 32 32 :fill "#9C7B52" :stroke "#3A3A3A" :stroke-width 2)
  (text (+ legend-x 444) (+ legend-y 114) "Dirt (5)" :fill "#B0B0B0" :font-size 22)))

(render "08_isometric_tilemap.png")
