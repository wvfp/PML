; isometric.pml - 等距投影（isometric）核心模块
; 提供：坐标转换、菱形瓦片绘制、等距瓦片地图渲染

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

; 等距坐标 → 屏幕坐标
; tile_w: 菱形宽度, tile_h: 菱形高度（通常 tile_w = 2 * tile_h）
; origin_x, origin_y: 地图中心在屏幕上的偏移
(define (iso-to-screen tx ty tile_w tile_h origin_x origin_y)
  (list
    (+ origin_x (* (/ tile_w 2) (- tx ty)))
    (+ origin_y (* (/ tile_h 2) (+ tx ty)))))

; ══════════════════════════════════════════════════════════════════════════════
; 基础菱形瓦片
; ═══════════════════════════════════════════════════════════════════════════════

; 绘制一个菱形（等距瓦片俯视面）
; cx, cy: 菱形中心屏幕坐标
; w, h: 菱形宽度和高度
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

; 瓦片顶面（菱形）
(define (draw-tile-top cx cy w h fill stroke sw)
  (draw-diamond-tile cx cy w h fill stroke sw))

; 瓦片左侧面（平行四边形）
(define (draw-tile-left cx cy w h depth fill stroke sw)
  (polygon (list
             (list (- cx (/ w 2)) cy)
             (list cx (+ cy (/ h 2)))
             (list cx (+ cy (/ h 2) depth))
             (list (- cx (/ w 2)) (+ cy depth)))
           :fill fill
           :stroke stroke
           :stroke-width sw))

; 瓦片右侧面（平行四边形）
(define (draw-tile-right cx cy w h depth fill stroke sw)
  (polygon (list
             (list (+ cx (/ w 2)) cy)
             (list cx (+ cy (/ h 2)))
             (list cx (+ cy (/ h 2) depth))
             (list (+ cx (/ w 2)) (+ cy depth)))
           :fill fill
           :stroke stroke
           :stroke-width sw))

; 完整的 3D 等距方块瓦片
(define (draw-tile-full cx cy w h depth top-fill left-fill right-fill stroke sw)
  (group
    (draw-tile-left cx cy w h depth left-fill stroke sw)
    (draw-tile-right cx cy w h depth right-fill stroke sw)
    (draw-tile-top cx cy w h top-fill stroke sw)))

; ═══════════════════════════════════════════════════════════════════════════════
; 预设瓦片类型
; ═══════════════════════════════════════════════════════════════════════════════

; 草地瓦片（绘本风：柔和绿、小草点缀、顶面微亮）
(define (draw-tile-grass cx cy w h depth stroke sw)
  (group
    (draw-tile-full cx cy w h depth
                    "#7CB342" "#558B2F" "#33691E"
                    stroke sw)
    ; 顶面：几簇小草，用细长三角形
    (polygon (list
               (list cx (- cy (* h 0.15)))
               (list (+ cx 4) (- cy (* h 0.05)))
               (list (- cx 3) (- cy (* h 0.05))))
             :fill "#9CCC65" :stroke "none")
    (polygon (list
               (list (- cx (* w 0.18)) (+ cy (* h 0.05)))
               (list (- cx (* w 0.08)) (+ cy (* h 0.15)))
               (list (- cx (* w 0.22)) (+ cy (* h 0.12))))
             :fill "#9CCC65" :stroke "none")
    (polygon (list
               (list (+ cx (* w 0.15)) cy)
               (list (+ cx (* w 0.22)) (+ cy (* h 0.08)))
               (list (+ cx (* w 0.10)) (+ cy (* h 0.08))))
             :fill "#AED581" :stroke "none")))

; 泥土瓦片（绘本风：暖棕色、小土块、裂缝）
(define (draw-tile-dirt cx cy w h depth stroke sw)
  (group
    (draw-tile-full cx cy w h depth
                    "#C49A6C" "#8D6E63" "#6D4C41"
                    stroke sw)
    ; 顶面小土块
    (ellipse (- cx (* w 0.12)) (+ cy (* h 0.05)) 6 3 :fill "#A1887F" :stroke "none")
    (ellipse (+ cx (* w 0.15)) (- cy (* h 0.08)) 5 2.5 :fill "#8D6E63" :stroke "none")
    ; 小裂缝
    (line cx cy (+ cx 6) (+ cy 2) :stroke "#5D4037" :stroke-width 0.8)))

; 石头瓦片（绘本风：暖灰、石块拼接、裂纹）
(define (draw-tile-stone cx cy w h depth stroke sw)
  (group
    (draw-tile-full cx cy w h depth
                    "#B0BEC5" "#78909C" "#546E7A"
                    stroke sw)
    ; 顶面：两块大石板
    (polygon (list
               (list (- cx (* w 0.15)) (- cy (* h 0.05)))
               (list cx (- cy (* h 0.15)))
               (list (+ cx (* w 0.05)) cy)
               (list (- cx (* w 0.18)) (+ cy (* h 0.05))))
             :fill "#CFD8DC" :stroke "#546E7A" :stroke-width 0.7)
    (polygon (list
               (list cx (- cy (* h 0.15)))
               (list (+ cx (* w 0.22)) (- cy (* h 0.02)))
               (list (+ cx (* w 0.12)) (+ cy (* h 0.12)))
               (list (- cx (* w 0.05)) cy))
             :fill "#B0BEC5" :stroke "#546E7A" :stroke-width 0.7)
    ; 裂纹
    (line (- cx 4) cy cx (+ cy 3) :stroke "#455A64" :stroke-width 0.7)))

; 路径瓦片（绘本风：石板路、接缝、缝隙小草）
(define (draw-tile-path cx cy w h stroke sw)
  (group
    (draw-diamond-tile cx cy w h "#D7CCC8" stroke sw)
    ; 中央石板
    (polygon (list
               (list (- cx (* w 0.12)) (- cy (* h 0.08)))
               (list (+ cx (* w 0.12)) (- cy (* h 0.08)))
               (list (+ cx (* w 0.12)) (+ cy (* h 0.08)))
               (list (- cx (* w 0.12)) (+ cy (* h 0.08))))
             :fill "#EFEBE9" :stroke "#8D6E63" :stroke-width 0.8)
    ; 石板接缝里长出的小草
    (polygon (list
               (list (+ cx (* w 0.14)) (- cy (* h 0.02)))
               (list (+ cx (* w 0.18)) (- cy (* h 0.08)))
               (list (+ cx (* w 0.16)) (+ cy (* h 0.02))))
             :fill "#7CB342" :stroke "none")))

; 沙地瓦片（绘本风：柔和黄沙、沙丘线、小石子）
(define (draw-tile-sand cx cy w h depth stroke sw)
  (group
    (draw-tile-full cx cy w h depth
                    "#E6D690" "#C9B66A" "#A8954E"
                    stroke sw)
    ; 沙丘波纹
    (line (- cx (* w 0.15)) cy (+ cx (* w 0.05)) (+ cy (* h 0.05))
          :stroke "#D4C87A" :stroke-width 1.5)
    (line (+ cx (* w 0.02)) (- cy (* h 0.08)) (+ cx (* w 0.18)) (- cy (* h 0.02))
          :stroke "#D4C87A" :stroke-width 1.5)
    ; 小石子
    (circle (- cx (* w 0.12)) (+ cy (* h 0.08)) 1.5 :fill "#A1887F" :stroke "none")
    (circle (+ cx (* w 0.14)) (- cy (* h 0.06)) 1.2 :fill "#8D6E63" :stroke "none")))

; 水面瓦片（绘本风：柔和水蓝、波浪弧线、高光）
(define (draw-tile-water cx cy w h stroke sw)
  (group
    (draw-diamond-tile cx cy w h "#64B5F6" stroke sw)
    ; 水面深浅渐变感：用椭圆做水底亮斑
    (ellipse (- cx (* w 0.08)) (+ cy (* h 0.05)) 10 4 :fill "#90CAF9" :stroke "none")
    ; 波纹高光（折线模拟波浪）
    (polygon (list
               (list (- cx (* w 0.12)) (- cy (* h 0.02)))
               (list (- cx (* w 0.06)) (+ cy (* h 0.02)))
               (list cx (- cy (* h 0.02)))
               (list (+ cx (* w 0.06)) (+ cy (* h 0.02)))
               (list (+ cx (* w 0.12)) (- cy (* h 0.02))))
             :stroke "#E3F2FD" :stroke-width 1.5 :fill "none")
    ; 小反光点
    (circle (+ cx (* w 0.1)) (- cy (* h 0.08)) 1.5 :fill "#FFFFFF" :stroke "none")))

; ═══════════════════════════════════════════════════════════════════════════════
; 地图渲染
; ═══════════════════════════════════════════════════════════════════════════════

; 渲染整个 tilemap
; map-grid: 二维列表，每个元素是瓦片类型编号
; tile_w, tile_h: 菱形尺寸
; depth: 瓦片厚度
; origin_x, origin_y: 地图中心屏幕坐标
; stroke, sw: 描边
(define (render-tilemap map-grid tile_w tile_h depth origin_x origin_y stroke sw)
  (render-tilemap-iter map-grid tile_w tile_h depth origin_x origin_y stroke sw 0 0))

; 迭代渲染：所有状态通过参数传递，避免局部 define
(define (render-tilemap-iter map-grid tile_w tile_h depth ox oy stroke sw row-idx col-idx)
  (if (< row-idx (length map-grid))
      (if (< col-idx (length (car map-grid)))
          (begin
            (add (draw-tile-by-type
                   (list-ref (list-ref map-grid row-idx) col-idx)
                   (car (iso-to-screen col-idx row-idx tile_w tile_h ox oy))
                   (list-ref (iso-to-screen col-idx row-idx tile_w tile_h ox oy) 1)
                   tile_w tile_h depth stroke sw))
            (render-tilemap-iter map-grid tile_w tile_h depth ox oy stroke sw row-idx (+ col-idx 1)))
          (render-tilemap-iter map-grid tile_w tile_h depth ox oy stroke sw (+ row-idx 1) 0))))

; 根据类型绘制瓦片
(define (draw-tile-by-type tile-type cx cy w h depth stroke sw)
  (cond
    ((= tile-type 0) (draw-tile-grass cx cy w h depth stroke sw))
    ((= tile-type 1) (draw-tile-water cx cy w h stroke sw))
    ((= tile-type 2) (draw-tile-sand cx cy w h depth stroke sw))
    ((= tile-type 3) (draw-tile-path cx cy w h stroke sw))
    ((= tile-type 4) (draw-tile-stone cx cy w h depth stroke sw))
    ((= tile-type 5) (draw-tile-dirt cx cy w h depth stroke sw))
    (else (draw-tile-grass cx cy w h depth stroke sw))))
