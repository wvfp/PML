; 示例 2：多边形与路径
; 本例用 polygon 绘制星形、心形和箭头。
; 注意：当前 C++ 端口尚未实现 path 原语，原本想展示的贝塞尔路径（心形、箭头）
; 这里使用 polygon / line 做近似，仍可说明复杂轮廓的构造思路。

(set-backend! "skia")
(canvas 400 300 :bg "#FFF9DB")

; 五角星：list 里嵌套 (x y) 点列表
(define star-points
  (list (list 200 50)
        (list 240 140)
        (list 340 140)
        (list 260 200)
        (list 290 290)
        (list 200 240)
        (list 110 290)
        (list 140 200)
        (list 60 140)
        (list 160 140)))

(add (polygon star-points
              :fill "#FFD43B"
              :stroke "#F08C00"
              :stroke-width 3))

; 心形：用多边形近似贝塞尔曲线轮廓
(define heart-points
  (list (list 100 180)
        (list 50 120)
        (list 50 90)
        (list 70 70)
        (list 100 100)
        (list 130 70)
        (list 150 90)
        (list 150 120)))

(add (polygon heart-points
              :fill "#FF6B6B"
              :stroke "#C92A2A"
              :stroke-width 2))

; 箭头：用多边形近似
(define arrow-points
  (list (list 260 80)
        (list 340 150)
        (list 300 150)
        (list 300 230)
        (list 220 230)
        (list 220 150)
        (list 180 150)))

(add (polygon arrow-points
              :fill "#74C0FC"
              :stroke "#1864AB"
              :stroke-width 2))

(render "02_polygon_and_path.png")
