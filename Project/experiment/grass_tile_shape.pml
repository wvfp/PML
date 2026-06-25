; grass_tile_shape.pml — 草地六边形瓦片 (材质与形状分离)
; 顶面: 六边形 + shader
; 侧面: 3 个平行四边形, 材质从 grass_side_tex.pml 导入

(provide hexagon draw-grass-tile)

(import "grass_top_tex.pml" as top)
(import "grass_side_tex.pml" as side)

; ═══════════════════════════════════════════════════════════════════════════════
; 顶面六边形 (等距 flat-topped, 6 顶点顺时针)
;   v0:左上  v1:右上  v2:右  v3:右下  v4:左下  v5:左
; ═══════════════════════════════════════════════════════════════════════════════

(define (hexagon cx cy w h)
  (polygon (list
             (list (- cx (/ w 4)) (- cy (/ h 2)))
             (list (+ cx (/ w 4)) (- cy (/ h 2)))
             (list (+ cx (/ w 2)) cy)
             (list (+ cx (/ w 4)) (+ cy (/ h 2)))
             (list (- cx (/ w 4)) (+ cy (/ h 2)))
             (list (- cx (/ w 2)) cy))
           :fill "#3D8C2E" :stroke "none" :stroke-width 0))

; ═══════════════════════════════════════════════════════════════════════════════
; apply-material: 将材质列表应用到平行四边形
; material = ((name color v_max) ...) 从底到顶排列, v_max 递减
; p0,p1 = 顶边两点 (v=0), p3,p2 = 底边两点 (v=1)
; ══════════════════════════════════════════════════════════════════════════════

(define (apply-material p0x p0y p1x p1y p3x p3y p2x p2y material prev-v)
  (if (null? material)
      '()
      (let* ((entry   (car material))
             (color   (list-ref entry 1))
             (v-cur   (list-ref entry 2))
             (lx1     (+ p0x (* prev-v (- p3x p0x))))
             (ly1     (+ p0y (* prev-v (- p3y p0y))))
             (rx1     (+ p1x (* prev-v (- p2x p1x))))
             (ry1     (+ p1y (* prev-v (- p2y p1y))))
             (lx2     (+ p0x (* v-cur (- p3x p0x))))
             (ly2     (+ p0y (* v-cur (- p3y p0y))))
             (rx2     (+ p1x (* v-cur (- p2x p1x))))
             (ry2     (+ p1y (* v-cur (- p2y p1y)))))
        (cons (polygon (list (list lx1 ly1) (list rx1 ry1)
                             (list rx2 ry2) (list lx2 ly2))
                       :fill color :stroke "none" :stroke-width 0)
              (apply-material p0x p0y p1x p1y p3x p3y p2x p2y
                              (cdr material) v-cur)))))

; ═══════════════════════════════════════════════════════════════════════════════
; 左前面平行四边形: v5(左) → v4(左下) 边向下延伸 td
; p0=v5(top-left of face), p1=v4(bottom-right), p3=p0+td, p2=p1+td 
; ══════════════════════════════════════════════════════════════════════════════

(define (left-front-face cx cy w h td)
  (define p0x (- cx (/ w 2)))
  (define p0y cy)
  (define p1x (- cx (/ w 4)))
  (define p1y (+ cy (/ h 2)))
  (define p3x p0x)
  (define p3y (+ p0y td))
  (define p2x p1x)
  (define p2y (+ p1y td))
  (apply group (apply-material p0x p0y p1x p1y p3x p3y p2x p2y side/left-material 1.0)))

; ══════════════════════════════════════════════════════════════════════════════
; 右前面平行四边形: v1(右) → v2(右下) 边向下延伸 td
; p0=v1(top-right of face), p1=v2(bottom-left), p3=p0+td, p2=p1+td
; ══════════════════════════════════════════════════════════════════════════════

(define (right-front-face cx cy w h td)
  (define p0x (+ cx (/ w 2)))
  (define p0y cy)
  (define p1x (+ cx (/ w 4)))
  (define p1y (+ cy (/ h 2)))
  (define p3x p0x)
  (define p3y (+ p0y td))
  (define p2x p1x)
  (define p2y (+ p1y td))
  (apply group (apply-material p0x p0y p1x p1y p3x p3y p2x p2y side/right-material 1.0)))

; ══════════════════════════════════════════════════════════════════════════════
; 底面平行四边形: v4(左下) → v3(右下) 边向下延伸 td
; ══════════════════════════════════════════════════════════════════════════════

(define (bottom-face cx cy w h td)
  (define p0x (- cx (/ w 4)))
  (define p0y (+ cy (/ h 2)))
  (define p1x (+ cx (/ w 4)))
  (define p1y (+ cy (/ h 2)))
  (define p3x p0x)
  (define p3y (+ p0y td))
  (define p2x p1x)
  (define p2y (+ p1y td))
  (apply group (apply-material p0x p0y p1x p1y p3x p3y p2x p2y side/bottom-material 1.0)))
; ══════════════════════════════════════════════════════════════════════════════
; 完整瓦片
; ══════════════════════════════════════════════════════════════════════════════

(define (draw-grass-tile cx cy tw th td shader)
  (group
    (left-front-face cx cy tw th td)
    (right-front-face cx cy tw th td)
    (bottom-face cx cy tw th td)
    (apply-shader! (hexagon cx cy tw th) shader)))
