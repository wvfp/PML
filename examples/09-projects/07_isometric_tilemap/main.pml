; main.pml - 2.5D 等距 tilemap 地图素材
; 展示等距投影瓦片地图 + 装饰物

(import "isometric.pml" as iso)
(import "decorations.pml" as deco)
(import "map_data.pml" as data)

(set-backend! "skia")
(canvas 800 700 :bg "#2C3E50")

; ══════════════════════════════════════════════════════════════════════════════
; 配置参数
; ═══════════════════════════════════════════════════════════════════════════════

(define tile-w 64)
(define tile-h 32)
(define tile-depth 16)
(define stroke "#333333")
(define stroke-w 1)
(define origin-x 400)
(define origin-y 200)

; ═══════════════════════════════════════════════════════════════════════════════
; 标题
; ═══════════════════════════════════════════════════════════════════════════════

(add (text 400 30 "2.5D Isometric Tilemap" :fill "#ECF0F1" :font-size 22 :align 'center))
(add (text 400 55 "Grass | Water | Sand | Path | Stone | Dirt" :fill "#BDC3C7" :font-size 12 :align 'center))

; ══════════════════════════════════════════════════════════════════════════════
; 渲染瓦片地图
; ═══════════════════════════════════════════════════════════════════════════════

(iso/render-tilemap data/map-grid tile-w tile-h tile-depth origin-x origin-y stroke stroke-w)

; ══════════════════════════════════════════════════════════════════════════════
; 渲染装饰物
; ═══════════════════════════════════════════════════════════════════════════════

; 渲染装饰物 - 所有状态通过参数传递
(define (place-decoration dec tile-w tile-h origin-x origin-y)
  (define type (list-ref dec 0))
  (define col (list-ref dec 1))
  (define row (list-ref dec 2))
  (define pos (iso/iso-to-screen col row tile-w tile-h origin-x origin-y))
  (define cx (car pos))
  (define cy (list-ref pos 1))
  (cond
    ((eq? type 'tree) (add (deco/draw-tree cx cy)))
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

; ══════════════════════════════════════════════════════════════════════════════
; 图例
; ═══════════════════════════════════════════════════════════════════════════════

(define legend-x 30)
(define legend-y 620)

(add (text legend-x legend-y "Legend:" :fill "#ECF0F1" :font-size 14))

(add (group
  (rect legend-x (+ legend-y 22) 16 16 :fill "#6DBF47" :stroke "#333" :stroke-width 1)
  (text (+ legend-x 22) (+ legend-y 35) "Grass (0)" :fill "#BDC3C7" :font-size 11)))
(add (group
  (rect (+ legend-x 100) (+ legend-y 22) 16 16 :fill "#4A90D9" :stroke "#333" :stroke-width 1)
  (text (+ legend-x 122) (+ legend-y 35) "Water (1)" :fill "#BDC3C7" :font-size 11)))
(add (group
  (rect (+ legend-x 200) (+ legend-y 22) 16 16 :fill "#E8D68C" :stroke "#333" :stroke-width 1)
  (text (+ legend-x 222) (+ legend-y 35) "Sand (2)" :fill "#BDC3C7" :font-size 11)))
(add (group
  (rect legend-x (+ legend-y 44) 16 16 :fill "#D4A76A" :stroke "#333" :stroke-width 1)
  (text (+ legend-x 22) (+ legend-y 57) "Path (3)" :fill "#BDC3C7" :font-size 11)))
(add (group
  (rect (+ legend-x 100) (+ legend-y 44) 16 16 :fill "#9E9E9E" :stroke "#333" :stroke-width 1)
  (text (+ legend-x 122) (+ legend-y 57) "Stone (4)" :fill "#BDC3C7" :font-size 11)))
(add (group
  (rect (+ legend-x 200) (+ legend-y 44) 16 16 :fill "#C4A265" :stroke "#333" :stroke-width 1)
  (text (+ legend-x 222) (+ legend-y 57) "Dirt (5)" :fill "#BDC3C7" :font-size 11)))

(render "07_isometric_tilemap.png")
