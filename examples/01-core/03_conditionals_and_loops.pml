; 示例 3：条件表达式、逻辑运算与列表操作
; 使用 if/cond/and/or，递归生成列表，map/filter/reduce 绘制条形图。

(define (range-list start end)
  (if (>= start end)
      '()
      (cons start (range-list (+ start 1) end))))

(define data (range-list 1 8))
(println "data = " data)

; 使用 filter 只保留偶数索引的值
(define even-data (filter even? data))
(println "even = " even-data)

; 使用 reduce 求和
(define total (reduce + 0 data))
(println "total = " (number->string total))

; 画布
(canvas 500 300 :bg "#ECF0F1")

; 绘制条形图
(define bar-width 40)
(define spacing 20)
(define base-y 250)

(for-each
  (lambda (i)
    (define value (+ 1 i))
    (define x (+ 40 (* i (+ bar-width spacing))))
    (define h (* value 25))
    (define color
      (cond
        ((and (>= value 6) (<= value 8)) "#E74C3C")
        ((or (= value 3) (= value 5)) "#F39C12")
        (else "#3498DB")))
    (add (rect x (- base-y h) bar-width h
               :fill color
               :stroke "#2C3E50"
               :stroke-width 1))
    (add (text (+ x (/ bar-width 2)) (+ base-y 20)
               (number->string value)
               :fill "#2C3E50"
               :font-size 14)))
  (range 0 7))

(add (text 250 30 "条件与循环示例" :fill "#2C3E50" :font-size 20))

(render "03_conditionals_and_loops.png")
