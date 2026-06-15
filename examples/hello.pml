; PML Phase 1 示例 — 纯计算程序
; 验证：函数定义、递归、条件、列表操作、字符串格式化

; ============================
; 1. 基础算术与变量
; ============================
(define pi 3.14159265358979)
(println "pi =" pi)
(println "pi * 2 =" (* pi 2))

; ============================
; 2. 函数定义与递归
; ============================
(define (factorial n)
  (if (= n 0) 1
      (* n (factorial (- n 1)))))

(println "5! =" (factorial 5))
(println "10! =" (factorial 10))

(define (fibonacci n)
  (cond
    ((= n 0) 0)
    ((= n 1) 1)
    (else (+ (fibonacci (- n 1))
             (fibonacci (- n 2))))))

(println "fib(10) =" (fibonacci 10))

; ============================
; 3. let / let* 局部绑定
; ============================
(define result
  (let ((a 10)
        (b 20))
    (let* ((sum (+ a b))
           (product (* a b)))
      (list sum product))))

(println "let result:" result)

; ============================
; 4. 列表操作
; ============================
(define numbers (list 1 2 3 4 5 6 7 8 9 10))

(define squares (map (lambda (x) (* x x)) numbers))
(println "squares:" squares)

(define evens (filter (lambda (x) (= 0 (% x 2))) numbers))
(println "evens:" evens)

(define total (reduce + 0 numbers))
(println "sum 1..10 =" total)

; ============================
; 5. 高阶函数与闭包
; ============================
(define (make-counter)
  (define count 0)
  (lambda ()
    (set! count (+ count 1))
    count))

(define counter (make-counter))
(println "counter:" (counter) (counter) (counter))

; ============================
; 6. 字符串格式化
; ============================
(define name "PML")
(define version "0.1.0")
(println (format "~a version ~a — Phase 1 complete!" name version))

; ============================
; 7. do 循环
; ============================
(define sum-result
  (do ((i 1 (+ i 1))
       (acc 0 (+ acc i)))
      ((> i 100) acc)))
(println "sum 1..100 =" sum-result)

; ============================
; 8. quasiquote
; ============================
(define x 42)
(define items (list "a" "b" "c"))
(println "quasiquote:" `(x is ,x and items are ,@items))

(println "\n=== All examples completed successfully ===")
