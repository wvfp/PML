; 示例 5：向量与哈希表
; 使用 make-vector/vector-set! 和 make-hash/hash-set!，
; 用哈希表作为颜色配置绘制网格。

; 创建一个 4x4 的向量网格，每个单元格存储数值
(define grid (make-vector 16 0))

(for-each
  (lambda (i)
    (vector-set! grid i (modulo (+ i 1) 4)))
  (range 0 16))

; 用哈希表保存颜色配置（键为字符串）
(define colors (make-hash))
(hash-set! colors "0" "#E74C3C")
(hash-set! colors "1" "#3498DB")
(hash-set! colors "2" "#2ECC71")
(hash-set! colors "3" "#F39C12")

(println "grid length = " (number->string (vector-length grid)))
(println "colors keys = " (hash-keys colors))

; 画布
(canvas 320 320 :bg "#ECF0F1")

(define cell-size 60)
(define margin 20)

(for-each
  (lambda (i)
    (define value (vector-ref grid i))
    (define row (quotient i 4))
    (define col (modulo i 4))
    (define x (+ margin (* col cell-size)))
    (define y (+ margin (* row cell-size)))
    (add (rect x y cell-size cell-size
               :fill (hash-ref colors (number->string value) "#BDC3C7")
               :stroke "#2C3E50"
               :stroke-width 1))
    (add (text (+ x (/ cell-size 2)) (+ y (/ cell-size 2))
               (number->string value)
               :fill "#FFFFFF"
               :font-size 20)))
  (range 0 16))

(add (text 160 300 "向量 + 哈希表示例" :fill "#2C3E50" :font-size 18))

(render "05_vectors_and_hashes.png")
