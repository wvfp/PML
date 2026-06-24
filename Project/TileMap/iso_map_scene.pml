; iso_map_scene.pml — 6×6 等距小地图场景（含过渡瓦片）
; 草地 + 泥土小径 + 石头平台 + 石头立方体
; 草地↔泥土/石头交界处使用过渡瓦片

(set-backend! "skia")
(canvas 2000 1200)

(import "grass_lib.pml" as grass)
(import "dirt_lib.pml" as dirt)
(import "stone_lib.pml" as stone)
(import "grass_dirt_transition.pml" as gd)
(import "grass_stone_transition.pml" as gs)
(import "stone_cube_lib.pml" as cube)

(define tw 300)
(define th 150)
(define td 54)

(define ox 1000)
(define oy 180)

; ═══════════════════════════════════════════════════════════════════════════════
; 地图数据: 0=grass, 1=dirt, 2=stone
; 6×6 网格
; ═══════════════════════════════════════════════════════════════════════════════

(define (tile-type row col)
  (cond
    ((and (= row 2) (or (= col 2) (= col 3))) 1)
    ((and (= row 3) (or (= col 2) (= col 3))) 1)
    ((and (= row 3) (or (= col 4) (= col 5))) 2)
    ((and (= row 4) (= col 3)) 1)
    ((and (= row 4) (or (= col 4) (= col 5))) 2)
    (else 0)))

; ══════════════════════════════════════════════════════════════════════════════
; 坐标转换
; ═══════════════════════════════════════════════════════════════════════════════

(define (iso-to-screen col row)
  (list
    (+ ox (* (/ tw 2) (- col row)))
    (+ oy (* (/ th 2) (+ col row)))))

; ═══════════════════════════════════════════════════════════════════════════════
; 判断是否为草地且相邻非草地
; ═══════════════════════════════════════════════════════════════════════════════

(define (is-grass-transition row col)
  (if (= (tile-type row col) 0)
      (let ((left-tt (if (> col 0) (tile-type row (- col 1)) -1))
            (right-tt (if (< col 5) (tile-type row (+ col 1)) -1)))
        (or (= left-tt 1) (= right-tt 1)
            (= left-tt 2) (= right-tt 2)))
      #f))

(define (transition-type row col)
  (if (is-grass-transition row col)
      (let ((left-tt (if (> col 0) (tile-type row (- col 1)) -1))
            (right-tt (if (< col 5) (tile-type row (+ col 1)) -1)))
        (cond
          ((or (= left-tt 1) (= right-tt 1)) 'dirt)
          ((or (= left-tt 2) (= right-tt 2)) 'stone)
          (else 'none)))
      'none))

; ═══════════════════════════════════════════════════════════════════════════════
; Pass 1: 地面瓦片
; ══════════════════════════════════════════════════════════════════════════════

(define (render-ground rows cols row-idx col-idx)
  (if (< row-idx rows)
      (if (< col-idx cols)
          (begin
            (define pos (iso-to-screen col-idx row-idx))
            (define cx (car pos))
            (define cy (list-ref pos 1))
            (define tt (tile-type row-idx col-idx))
            (define tr (transition-type row-idx col-idx))
            (cond
              ((= tt 0)
               (cond
                 ((eq? tr 'dirt) (add (gd/draw-grass-dirt-transition cx cy tw th td grass/grass-shader)))
                 ((eq? tr 'stone) (add (gs/draw-grass-stone-transition cx cy tw th td grass/grass-shader)))
                 (else (add (grass/draw-grass-tile cx cy tw th td grass/grass-shader)))))
              ((= tt 1) (add (dirt/draw-dirt-tile cx cy tw th td dirt/dirt-shader)))
              ((= tt 2) (add (stone/draw-stone-tile cx cy tw th td stone/stone-shader))))
            (render-ground rows cols row-idx (+ col-idx 1)))
          (render-ground rows cols (+ row-idx 1) 0))))

; ═══════════════════════════════════════════════════════════════════════════════
; Pass 2: 装饰 — 石头立方体
; ═══════════════════════════════════════════════════════════════════════════════

(define (render-decorations rows cols row-idx col-idx)
  (if (< row-idx rows)
      (if (< col-idx cols)
          (begin
            (if (and (= row-idx 3) (= col-idx 4))
                (begin
                  (define pos (iso-to-screen col-idx row-idx))
                  (define cx (car pos))
                  (define cy (list-ref pos 1))
                  (add (cube/draw-stone-cube cx cy tw th 150))))
            (render-decorations rows cols row-idx (+ col-idx 1)))
          (render-decorations rows cols (+ row-idx 1) 0))))

; ══════════════════════════════════════════════════════════════════════════════
; 渲染
; ═══════════════════════════════════════════════════════════════════════════════

(render-ground 6 6 0 0)
(render-decorations 6 6 0 0)

(render "iso_map_scene.png")
