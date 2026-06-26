; isometric.pml - 等距投影核心模块
; 提供：坐标转换、菱形瓦片、3D 瓦片面、地图渲染

(provide
  iso-to-screen
  draw-diamond-tile
  draw-tile-left
  draw-tile-right
  draw-tile-with-shader
  render-tilemap
  render-tilemap-decorations)

; ==========================================================================================================================================================================================================================================═
; 坐标转换
; ==========================================================================================================================================================================================================================================═

(define (iso-to-screen tx ty tile_w tile_h origin_x origin_y)
  (list
    (+ origin_x (* (/ tile_w 2) (- tx ty)))
    (+ origin_y (* (/ tile_h 2) (+ tx ty)))))

; ==========================================================================================================================================================================================================================================═
; 菱形顶面
; ==========================================================================================================================================================================================================================================═

(define (draw-diamond-tile cx cy w h fill stroke sw)
  (polygon (list
             (list cx (- cy (/ h 2)))
             (list (+ cx (/ w 2)) cy)
             (list cx (+ cy (/ h 2)))
             (list (- cx (/ w 2)) cy))
           :fill fill :stroke stroke :stroke-width sw))

; ==========================================================================================================================================================================================================================================═
; 侧面平行四边形
; ==========================================================================================================================================================================================================================================═

(define (draw-tile-left cx cy w h depth fill stroke sw)
  (polygon (list
             (list (- cx (/ w 2)) cy)
             (list cx (+ cy (/ h 2)))
             (list cx (+ cy (/ h 2) depth))
             (list (- cx (/ w 2)) (+ cy depth)))
           :fill fill :stroke stroke :stroke-width sw))

(define (draw-tile-right cx cy w h depth fill stroke sw)
  (polygon (list
             (list (+ cx (/ w 2)) cy)
             (list cx (+ cy (/ h 2)))
             (list cx (+ cy (/ h 2) depth))
             (list (+ cx (/ w 2)) (+ cy depth)))
           :fill fill :stroke stroke :stroke-width sw))

; ==========================================================================================================================================================================================================================================═
; 带 shader 的 3D 瓦片 (顶面用 shader，侧面用纯色)
; ==========================================================================================================================================================================================================================================═

(define (draw-tile-with-shader cx cy w h depth shader-handle
                                left-fill right-fill stroke sw)
  (group
    (draw-tile-left cx cy w h depth left-fill stroke sw)
    (draw-tile-right cx cy w h depth right-fill stroke sw)
    (apply-shader! (draw-diamond-tile cx cy w h "#FFFFFF" stroke sw)
                   shader-handle)))

; ==========================================================================================================================================================================================================================================═
; 地图渲染 (仅地面，不含装饰)
; ==========================================================================================================================================================================================================================================═

(define (render-tilemap map-grid tile_w tile_h depth
                        origin_x origin_y stroke sw
                        grass-sh dirt-sh stone-sh sand-sh)
  (render-tilemap-iter map-grid tile_w tile_h depth
                       origin_x origin_y stroke sw
                       grass-sh dirt-sh stone-sh sand-sh
                       0 0))

(define (render-tilemap-iter map-grid tile_w tile_h depth
                             ox oy stroke sw
                             grass-sh dirt-sh stone-sh sand-sh
                             row col)
  (if (< row (length map-grid))
      (if (< col (length (car map-grid)))
          (let* ((tile-type (list-ref (list-ref map-grid row) col))
                 (screen (iso-to-screen col row tile_w tile_h ox oy))
                 (sx (car screen))
                 (sy (list-ref screen 1))
                 (tile-shader
                   (cond
                     ((= tile-type 0) grass-sh)
                     ((= tile-type 1) dirt-sh)
                     ((= tile-type 2) stone-sh)
                     ((= tile-type 3) sand-sh)
                     (else grass-sh)))
                 (left-color
                   (cond
                     ((= tile-type 0) "#3A6B2E")
                     ((= tile-type 1) "#5A4535")
                     ((= tile-type 2) "#4A4B52")
                     ((= tile-type 3) "#A89060")
                     (else "#3A6B2E")))
                 (right-color
                   (cond
                     ((= tile-type 0) "#2D5424")
                     ((= tile-type 1) "#453528")
                     ((= tile-type 2) "#383940")
                     ((= tile-type 3) "#8A7448")
                     (else "#2D5424"))))
            (begin
              (add (draw-tile-with-shader sx sy tile_w tile_h depth
                                          tile-shader left-color right-color
                                          stroke sw))
              (render-tilemap-iter map-grid tile_w tile_h depth
                                   ox oy stroke sw
                                   grass-sh dirt-sh stone-sh sand-sh
                                   row (+ col 1))))
          (render-tilemap-iter map-grid tile_w tile_h depth
                               ox oy stroke sw
                               grass-sh dirt-sh stone-sh sand-sh
                               (+ row 1) 0))))

; ==========================================================================================================================================================================================================================================═
; 装饰渲染
; ==========================================================================================================================================================================================================================================═

(define (render-tilemap-decorations map-grid deco-list tile_w tile_h
                                     origin_x origin_y
                                     stroke sw draw-deco-fn)
  (render-deco-iter deco-list tile_w tile_h origin_x origin_y
                    stroke sw draw-deco-fn))

(define (render-deco-iter deco-list tile_w tile_h ox oy stroke sw draw-fn)
  (if (> (length deco-list) 0)
      (let* ((deco (car deco-list))
             (dtype (list-ref deco 0))
             (dx (list-ref deco 1))
             (dy (list-ref deco 2))
             (screen (iso-to-screen dx dy tile_w tile_h ox oy))
             (sx (car screen))
             (sy (list-ref screen 1)))
        (begin
          (add (draw-fn dtype sx sy))
          (render-deco-iter (cdr deco-list) tile_w tile_h ox oy stroke sw draw-fn)))))
