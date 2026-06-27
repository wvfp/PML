;; ========================================
;; PML 内置扩展：div, ->double, ->int, dotimes
;; ========================================
;; 运行: pml.exe 09_builtin_extensions.pml

(set-backend! "skia")
(canvas 400 300 :bg "#1a1a2e")

;; ---- Phase 1: div (整数除法) ----
(print "=== div ===")
(print "(div 10 3) =" (div 10 3))               ;; → 3
(print "(div 20 4) =" (div 20 4))               ;; → 5
(print "= (quotient 15 4):" (= (div 15 4) (quotient 15 4)))  ;; → #t

;; ---- Phase 2: ->double / ->int (类型转换) ----
(print "=== Type Conversion ===")
(print "(->double 5) =" (->double 5))            ;; → 5.0
(print "(->int 3.14) =" (->int 3.14))            ;; → 3

;; ---- Phase 3: dotimes / for (迭代) ----
(print "=== dotimes ===")
(let ((sum 0))
  ;; 0 到 4 求和
  (dotimes (i 5)
    (set! sum (+ sum i))
    (print "i =" i ", cumulative sum =" sum))
  (print "sum 0..4 =" sum))

(print "=== for ===")
(for (i 0 5)
  (print "for i =" i))
(for (i 0 10 3)
  (print "for step 3 i =" i))

;; ---- 可视化：用 dotimes 画一排圆 ----
(add (rect 0 0 400 300 :fill "#16213e"))
(let ((colors (list "#e94560" "#0f3460" "#53d769" "#f9ca24" "#6c5ce7" "#fd79a8" "#00cec9" "#fdcb6e" "#a29bfe" "#55efc4")))
  (dotimes (i 10)
    (let ((x (+ 20 (* i 36)))
          (color (nth i colors)))
      (add (circle x 150 14 :fill color :opacity 0.85)))))

(render "09_builtin_extensions.png")
