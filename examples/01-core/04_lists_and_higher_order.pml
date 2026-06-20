; 示例 4：列表与高阶函数
; 用 range/map 创建一排圆，用 reduce 计算总面积并显示。

(define pi 3.14159)

(define (circle-area r)
  (* pi r r))

; 创建半径列表
(define radii (range 1 7))
(println "radii = " radii)

; 用 map 生成圆对象列表
(define circles
  (map (lambda (r)
         (circle (+ 60 (* r 50)) 100 (* r 7)
                 :fill (if (even? r) "#9B59B6" "#1ABC9C")
                 :stroke "#2C3E50"
                 :stroke-width 2))
       radii))

; 用 reduce 计算总面积
(define total-area
  (reduce + 0 (map circle-area radii)))
(println "total area = " (number->string total-area))

; 创建画布并把所有圆加入
(canvas 450 250 :bg "#FDFEFE")

(for-each add circles)

; 显示总面积
(add (text 225 200
           (string-append "总面积: " (number->string total-area))
           :fill "#2C3E50"
           :font-size 18))

(render "04_lists_and_higher_order.png")
