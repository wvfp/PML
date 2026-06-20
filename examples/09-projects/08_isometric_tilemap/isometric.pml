; isometric.pml - 2.5D 等距投影核心模块（Stardew Valley 风格）
; tile: 256×128, depth: 40

(provide
  iso-to-screen
  draw-diamond-tile
  draw-tile-top
  draw-tile-left
  draw-tile-right
  draw-tile-full
  draw-tile-water
  draw-tile-sand
  draw-tile-path
  draw-tile-stone
  draw-tile-grass
  draw-tile-dirt
  render-tilemap)

; ═══════════════════════════════════════════════════════════════════════════════
; 坐标转换
; ═══════════════════════════════════════════════════════════════════════════════

(define (iso-to-screen tx ty tile_w tile_h origin_x origin_y)
  (list
    (+ origin_x (* (/ tile_w 2) (- tx ty)))
    (+ origin_y (* (/ tile_h 2) (+ tx ty)))))

; ═══════════════════════════════════════════════════════════════════════════════
; 基础菱形瓦片
; ═══════════════════════════════════════════════════════════════════════════════

(define (draw-diamond-tile cx cy w h fill stroke sw)
  (polygon (list
             (list cx (- cy (/ h 2)))
             (list (+ cx (/ w 2)) cy)
             (list cx (+ cy (/ h 2)))
             (list (- cx (/ w 2)) cy))
           :fill fill
           :stroke stroke
           :stroke-width sw))

; ═══════════════════════════════════════════════════════════════════════════════
; 带厚度的 3D 瓦片（等距方块）
; ═══════════════════════════════════════════════════════════════════════════════

(define (draw-tile-top cx cy w h fill stroke sw)
  (draw-diamond-tile cx cy w h fill stroke sw))

(define (draw-tile-left cx cy w h depth fill stroke sw)
  (polygon (list
             (list (- cx (/ w 2)) cy)
             (list cx (+ cy (/ h 2)))
             (list cx (+ cy (/ h 2) depth))
             (list (- cx (/ w 2)) (+ cy depth)))
           :fill fill
           :stroke stroke
           :stroke-width sw))

(define (draw-tile-right cx cy w h depth fill stroke sw)
  (polygon (list
             (list (+ cx (/ w 2)) cy)
             (list cx (+ cy (/ h 2)))
             (list cx (+ cy (/ h 2) depth))
             (list (+ cx (/ w 2)) (+ cy depth)))
           :fill fill
           :stroke stroke
           :stroke-width sw))

(define (draw-tile-full cx cy w h depth top-fill left-fill right-fill stroke sw)
  (group
    ; 侧面保留极淡描边，顶面去描边让拼接更自然
    (draw-tile-left cx cy w h depth left-fill left-fill sw)
    (draw-tile-right cx cy w h depth right-fill right-fill sw)
    (draw-tile-top cx cy w h top-fill "none" 0)))

; ═══════════════════════════════════════════════════════════════════════════════
; 邻接查询与过渡绘制
; ═══════════════════════════════════════════════════════════════════════════════

(define (get-tile-at map-grid row col)
  (if (and (>= row 0) (< row (length map-grid))
           (>= col 0) (< col (length (car map-grid))))
      (list-ref (list-ref map-grid row) col)
      -1))

(define (neighbor-is? map-grid row col type)
  (or (= (get-tile-at map-grid (- row 1) col) type)
      (= (get-tile-at map-grid (+ row 1) col) type)
      (= (get-tile-at map-grid row (- col 1)) type)
      (= (get-tile-at map-grid row (+ col 1)) type)))

(define (neighbor-at? map-grid row col drow dcol type)
  (= (get-tile-at map-grid (+ row drow) (+ col dcol)) type))

(define (draw-edge-triangle cx cy w h edge color)
  (cond
    ((eq? edge 'ul)
     (polygon (list (list cx (- cy (/ h 2)))
                    (list (- cx (/ w 2)) cy)
                    (list (- cx (/ w 6)) (- cy (/ h 6))))
              :fill color :stroke "none"))
    ((eq? edge 'ur)
     (polygon (list (list cx (- cy (/ h 2)))
                    (list (+ cx (/ w 2)) cy)
                    (list (+ cx (/ w 6)) (- cy (/ h 6))))
              :fill color :stroke "none"))
    ((eq? edge 'll)
     (polygon (list (list (- cx (/ w 2)) cy)
                    (list cx (+ cy (/ h 2)))
                    (list (- cx (/ w 6)) (+ cy (/ h 6))))
              :fill color :stroke "none"))
    ((eq? edge 'lr)
     (polygon (list (list (+ cx (/ w 2)) cy)
                    (list cx (+ cy (/ h 2)))
                    (list (+ cx (/ w 6)) (+ cy (/ h 6))))
              :fill color :stroke "none"))))

(define (draw-transition-edges cx cy w h map-grid row col target-type color)
  (group
    (if (neighbor-at? map-grid row col -1 0 target-type) (draw-edge-triangle cx cy w h 'ul color) (circle cx cy 0 :fill "none"))
    (if (neighbor-at? map-grid row col 0 -1 target-type) (draw-edge-triangle cx cy w h 'ur color) (circle cx cy 0 :fill "none"))
    (if (neighbor-at? map-grid row col 0 1 target-type) (draw-edge-triangle cx cy w h 'll color) (circle cx cy 0 :fill "none"))
    (if (neighbor-at? map-grid row col 1 0 target-type) (draw-edge-triangle cx cy w h 'lr color) (circle cx cy 0 :fill "none"))))

; ═══════════════════════════════════════════════════════════════════════════════
; 预设瓦片类型（Stardew Valley 风格：低饱和、暖色、手绘细节）
; ═══════════════════════════════════════════════════════════════════════════════

(define (draw-tile-grass cx cy w h depth stroke sw)
  (group
    (draw-tile-full cx cy w h depth
                    "#6B8E41" "#4E6F2E" "#3E5A24"
                    stroke sw)
    ; 顶面：不规则草簇
    (polygon (list (list (- cx 20) (- cy 18))
                   (list (- cx 8) (- cy 28))
                   (list (- cx 2) (- cy 16))
                   (list (- cx 14) (- cy 8)))
             :fill "#7FA650" :stroke "none")
    (polygon (list (list (+ cx 8) (- cy 22))
                   (list (+ cx 22) (- cy 28))
                   (list (+ cx 28) (- cy 14))
                   (list (+ cx 14) (- cy 10)))
             :fill "#8CB85C" :stroke "none")
    (polygon (list (list (- cx 32) (+ cy 4))
                   (list (- cx 18) (- cy 4))
                   (list (- cx 10) (+ cy 6))
                   (list (- cx 26) (+ cy 12)))
             :fill "#5E7A36" :stroke "none")
    (polygon (list (list (+ cx 18) (+ cy 20))
                   (list (+ cx 34) (+ cy 10))
                   (list (+ cx 42) (+ cy 24))
                   (list (+ cx 26) (+ cy 32)))
             :fill "#7FA650" :stroke "none")
    ; 零星草叶
    (line (- cx 44) (- cy 6) (- cx 38) (- cy 14) :stroke "#4E6F2E" :stroke-width 2.4)
    (line (- cx 40) (- cy 8) (- cx 36) (- cy 16) :stroke "#4E6F2E" :stroke-width 2.4)
    (line (+ cx 44) (+ cy 2) (+ cx 50) (- cy 6) :stroke "#4E6F2E" :stroke-width 2.4)
    (line (- cx 12) (+ cy 28) (- cx 6) (+ cy 20) :stroke "#4E6F2E" :stroke-width 2.4)))

(define (draw-tile-dirt cx cy w h depth stroke sw)
  (group
    (draw-tile-full cx cy w h depth
                    "#9C7B52" "#6E4F33" "#5A3F28"
                    stroke sw)
    ; 土块和小石子
    (ellipse (- cx 24) (- cy 10) 22 10 :fill "#B08C62" :stroke "none")
    (ellipse (+ cx 20) (+ cy 14) 18 9 :fill "#856745" :stroke "none")
    (ellipse (- cx 8) (+ cy 26) 14 7 :fill "#A68158" :stroke "none")
    ; 裂缝
    (line (- cx 18) (- cy 4) (- cx 4) (+ cy 6) :stroke "#5A3F28" :stroke-width 2.4)
    (line (- cx 4) (+ cy 6) (+ cx 8) (+ cy 2) :stroke "#5A3F28" :stroke-width 2.4)
    ; 小石头
    (circle (- cx 36) (+ cy 4) 5 :fill "#6E4F33" :stroke "none")
    (circle (+ cx 36) (- cy 16) 4 :fill "#5A3F28" :stroke "none")))

(define (draw-tile-stone cx cy w h depth stroke sw)
  (group
    (draw-tile-full cx cy w h depth
                    "#7D858C" "#5A6168" "#484E54"
                    stroke sw)
    ; 石板拼接
    (polygon (list
               (list (- cx 22) (- cy 6))
               (list (- cx 6) (- cy 16))
               (list (+ cx 6) (- cy 8))
               (list (- cx 8) (+ cy 2)))
             :fill "#929AA1" :stroke "#484E54" :stroke-width 2.4)
    (polygon (list
               (list (+ cx 8) (- cy 10))
               (list (+ cx 28) (- cy 2))
               (list (+ cx 16) (+ cy 10))
               (list (- cx 4) cy))
             :fill "#70787F" :stroke "#484E54" :stroke-width 2.4)
    (polygon (list
               (list (- cx 28) (+ cy 12))
               (list (- cx 8) (+ cy 4))
               (list (+ cx 4) (+ cy 16))
               (list (- cx 14) (+ cy 26)))
             :fill "#8E969D" :stroke "#484E54" :stroke-width 2.4)
    ; 裂缝
    (line (- cx 14) (- cy 12) (- cx 8) (- cy 2) :stroke "#3D4247" :stroke-width 2.4)
    (line (+ cx 10) (+ cy 4) (+ cx 18) (+ cy 14) :stroke "#3D4247" :stroke-width 2.4)))

(define (draw-tile-path cx cy w h stroke sw)
  (group
    (draw-diamond-tile cx cy w h "#BFA77A" "none" 0)
    ; 石板拼接
    (polygon (list
               (list (- cx 16) (- cy 10))
               (list (+ cx 6) (- cy 18))
               (list (+ cx 18) (- cy 8))
               (list (- cx 2) cy))
             :fill "#D4C193" :stroke "#8C7048" :stroke-width 2.4)
    (polygon (list
               (list (- cx 24) (+ cy 4))
               (list (- cx 4) (- cy 6))
               (list (+ cx 16) (+ cy 4))
               (list (- cx 4) (+ cy 14)))
             :fill "#CBB682" :stroke "#8C7048" :stroke-width 2.4)
    (polygon (list
               (list (+ cx 4) (+ cy 12))
               (list (+ cx 24) (+ cy 4))
               (list (+ cx 36) (+ cy 14))
               (list (+ cx 16) (+ cy 24)))
             :fill "#D4C193" :stroke "#8C7048" :stroke-width 2.4)
    ; 缝隙小草
    (polygon (list (list (+ cx 20) (- cy 6)) (list (+ cx 26) (- cy 14)) (list (+ cx 30) (- cy 4)))
             :fill "#5E7A36" :stroke "none")
    (polygon (list (list (- cx 26) (+ cy 6)) (list (- cx 20) (+ cy 14)) (list (- cx 14) (+ cy 6)))
             :fill "#5E7A36" :stroke "none")))

(define (draw-tile-sand cx cy w h depth stroke sw)
  (group
    (draw-tile-full cx cy w h depth
                    "#D4C298" "#A8946D" "#8F7D5A"
                    stroke sw)
    ; 沙丘波纹
    (line (- cx 44) (- cy 2) (- cx 16) (+ cy 10) :stroke "#C4B288" :stroke-width 5)
    (line (+ cx 8) (- cy 20) (+ cx 36) (- cy 8) :stroke "#C4B288" :stroke-width 5)
    (line (- cx 20) (+ cy 20) (+ cx 8) (+ cy 32) :stroke "#B9A67D" :stroke-width 5)
    ; 小贝壳/石子
    (ellipse (- cx 30) (+ cy 8) 8 5 :fill "#E8DCC4" :stroke "#A8946D" :stroke-width 1.6)
    (circle (+ cx 34) (- cy 6) 5 :fill "#B9A67D" :stroke "none")
    ; 脚印
    (ellipse (- cx 4) (- cy 8) 6 3 :fill "#C4B288" :stroke "none")
    (ellipse (+ cx 4) (- cy 4) 6 3 :fill "#C4B288" :stroke "none")))

(define (draw-tile-water cx cy w h stroke sw)
  (group
    (draw-diamond-tile cx cy w h "#4A90A4" "none" 0)
    ; 水波亮斑
    (ellipse (- cx 18) (+ cy 4) 36 12 :fill "#6BA8BE" :stroke "none")
    ; 波纹
    (polygon (list
               (list (- cx 48) (- cy 4))
               (list (- cx 28) (+ cy 6))
               (list (- cx 8) (- cy 4))
               (list (+ cx 12) (+ cy 6))
               (list (+ cx 32) (- cy 4))
               (list (+ cx 48) (+ cy 4)))
             :stroke "#7FBDD2" :stroke-width 5 :fill "none")
    (polygon (list
               (list (- cx 40) (+ cy 12))
               (list (- cx 16) (+ cy 24))
               (list (+ cx 8) (+ cy 12))
               (list (+ cx 32) (+ cy 24))
               (list (+ cx 48) (+ cy 16)))
             :stroke "#7FBDD2" :stroke-width 4 :fill "none")
    ; 反光点
    (circle (+ cx 20) (- cy 14) 7 :fill "#B8D9E6" :stroke "none")
    (circle (- cx 30) (+ cy 16) 5 :fill "#B8D9E6" :stroke "none")))

; ═══════════════════════════════════════════════════════════════════════════════
; 地图渲染
; ═══════════════════════════════════════════════════════════════════════════════

(define (render-tilemap map-grid tile_w tile_h depth origin_x origin_y stroke sw)
  (render-tilemap-iter map-grid tile_w tile_h depth origin_x origin_y stroke sw 0 0))

(define (render-tilemap-iter map-grid tile_w tile_h depth ox oy stroke sw row-idx col-idx)
  (if (< row-idx (length map-grid))
      (if (< col-idx (length (car map-grid)))
          (begin
            (define pos (iso-to-screen col-idx row-idx tile_w tile_h ox oy))
            (add (draw-tile-by-type
                   map-grid row-idx col-idx
                   (car pos) (list-ref pos 1)
                   tile_w tile_h depth stroke sw))
            (render-tilemap-iter map-grid tile_w tile_h depth ox oy stroke sw row-idx (+ col-idx 1)))
          (render-tilemap-iter map-grid tile_w tile_h depth ox oy stroke sw (+ row-idx 1) 0))))

(define (draw-tile-by-type map-grid row col cx cy w h depth stroke sw)
  (define tile-type (get-tile-at map-grid row col))
  (cond
    ((= tile-type 0)
     (group
       (draw-tile-grass cx cy w h depth stroke sw)
       (draw-transition-edges cx cy w h map-grid row col 2 "#C4B288")
       (draw-transition-edges cx cy w h map-grid row col 3 "#8C7048")
       (draw-transition-edges cx cy w h map-grid row col 5 "#8C7048")))
    ((= tile-type 1) (draw-tile-water cx cy w h stroke sw))
    ((= tile-type 2)
     (group
       (draw-tile-sand cx cy w h depth stroke sw)
       (draw-transition-edges cx cy w h map-grid row col 1 "#3D7A8A")))
    ((= tile-type 3) (draw-tile-path cx cy w h stroke sw))
    ((= tile-type 4) (draw-tile-stone cx cy w h depth stroke sw))
    ((= tile-type 5) (draw-tile-dirt cx cy w h depth stroke sw))
    (else (draw-tile-grass cx cy w h depth stroke sw))))
