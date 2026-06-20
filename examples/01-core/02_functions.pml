; 示例 2：函数、lambda 与高阶函数
; 渲染三个不同半径的圆到 02_functions.png。

(define pi 3.14159)

; 计算圆面积
(define (area r)
  (* pi r r))

; lambda 形式
(define square (lambda (x) (* x x)))

; 高阶函数：函数组合
(define (compose f g)
  (lambda (x) (f (g x))))

(define area-from-diameter
  (compose area (lambda (d) (/ d 2))))

; 创建画布
(canvas 400 200 :bg "#F8F9FA")

; 使用 map 绘制不同半径的圆
(for-each
  (lambda (r)
    (add (circle (+ 80 (* r 3)) 100 r
                 :fill (cond
                         ((< r 25) "#E74C3C")
                         ((< r 45) "#3498DB")
                         (else "#2ECC71"))
                 :stroke "#2C3E50"
                 :stroke-width 2)))
  '(10 30 50))

; 在画面上方标注面积
(add (text 200 30
           (string-append "A=" (number->string (area 50)))
           :fill "#2C3E50"
           :font-size 18))

(render "02_functions.png")
