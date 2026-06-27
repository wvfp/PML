; 示例 8：Lisp 兼容性新语法
; 展示 Phase 1-3 新增的 Lisp 兼容函数与形参语法
; 运行：cd G:/Project/PML && build/debug/src/pml/cli/Debug/pml.exe examples/01-core/08_lisp_compat.pml

(println "===== Lisp 兼容性语法示例 =====")
(println)

; ============================================================
; 1. 访问器别名：first/second/third/rest
; ============================================================
(println "--- 1. 列表访问: first/second/third/rest ---")

(define data '(10 20 30 40 50))
(println "  data = " data)
(println "  (first data)  = " (first data))
(println "  (second data) = " (second data))
(println "  (third data)  = " (third data))
(println "  (rest data)   = " (rest data))

; nth — Lisp 标准参数顺序 (nth n list)
(println)
(println "  (nth 0 data)  = " (nth 0 data))
(println "  (nth 3 data)  = " (nth 3 data))

(println)

; ============================================================
; 2. 类型谓词：null/atom/consp/listp
; ============================================================
(println "--- 2. 类型谓词: null/atom/consp/listp ---")

(println "  (null ())    = " (null ()))        ; #t — 空列表
(println "  (null 42)    = " (null 42))         ; #f
(println "  (atom 42)    = " (atom 42))         ; #t — 原子
(println "  (atom '(1))  = " (atom '(1 2)))     ; #f — 序对
(println "  (consp '(1)) = " (consp '(1 2)))    ; #t — 序对
(println "  (consp ())   = " (consp ()))         ; #f
(println "  (listp '())  = " (listp ()))         ; #t — 列表
(println "  (listp '(1)) = " (listp '(1 2)))     ; #t

(println)

; ============================================================
; 3. 序列函数：find/position/count/remove/last/butlast
; ============================================================
(println "--- 3. 序列函数 ---")

(define scores '(85 92 78 92 88 92 70))
(println "  scores = " scores)

(println "  (find 92 scores) = " (find 92 scores))       ; 92
(println "  (find 99 scores) = " (find 99 scores))       ; nil
(println "  (position 92 scores) = " (position 92 scores)) ; 1
(println "  (count 92 scores) = " (count 92 scores))      ; 3
(println "  (remove 92 scores) = " (remove 92 scores))     ; (85 78 88 70)
(println "  (last scores)  = " (last scores))     ; 70
(println "  (butlast scores) = " (butlast scores))  ; (85 92 78 92 88 92)

(println)

; ============================================================
; 4. 字符串增强：string-upcase/downcase/trim
; ============================================================
(println "--- 4. 字符串处理 ---")

(println "  (string-upcase \"hello\")   = " (string-upcase "hello"))
(println "  (string-downcase \"HELLO\") = " (string-downcase "HELLO"))
(println "  (string-trim \"  hi  \")    = " (string-trim "  hi  "))

; 组合使用：
(define msg "  Hello World  ")
(println "  trimmed + upcase: " (string-upcase (string-trim msg)))

(println)

; ============================================================
; 5. &optional/&key/&rest 形参语法
; ============================================================
(println "--- 5. &optional/&key/&rest ---")

; &optional: 可选参数
(define greet (lambda (name &optional greeting)
  (println "  greet: " greeting ", " name "!")))
(greet "Alice")                     ; 不提供 greeting → 显示 ()
(greet "Bob" "Hi")                  ; 提供 greeting

; &optional 带默认值
(define greet2 (lambda (name &optional (greeting "Hello"))
  (string-append greeting ", " name "!")))
(println "  " (greet2 "Charlie"))    ; 用默认值 "Hello"
(println "  " (greet2 "Dave" "Hey")) ; 用提供的值

; &key: 关键字参数
(define make-point (lambda (&key x y)
  (list "point" x y)))
(println "  make-point :x 10 :y 20 => " (make-point :x 10 :y 20))

; &key 带默认值
(define configure (lambda (&key (host "localhost") (port 8080))
  (string-append host ":" (number->string port))))
(println "  " (configure :host "example.com"))               ; 用默认 port
(println "  " (configure :port 3000 :host "test.local"))     ; 全部提供

; &rest: 剩余参数
(define sum-all (lambda (&rest numbers)
  (apply + numbers)))
(println "  (sum-all 1 2 3 4 5) = " (sum-all 1 2 3 4 5))

; 混合使用 &optional + &key + &rest
(define advanced-fn (lambda (x &optional y &key verbose &rest extras)
  (list x y verbose extras)))
(println "  advanced (1 2 :verbose #t 'a 'b): " (advanced-fn 1 2 :verbose #t 'a 'b))
(println "  advanced (42): " (advanced-fn 42))

(println)

; ============================================================
; 6. 组合使用：实际数据处理示例
; ============================================================
(println "--- 6. 组合使用示例 ---")

; 用 &optional + string-trim 处理姓名格式化
(define format-name (lambda (name &optional (prefix "Mr."))
  (string-append prefix " " (string-trim name))))
(println "  " (format-name "  Alice  "))
(println "  " (format-name "Bob" "Dr."))

; 用 find/first/last 分析成绩数据
(define exam-scores '(88 72 95 65 95 80))
(println "  exam-scores: " exam-scores)
(println "  first: " (first exam-scores))
(println "  last:  " (last exam-scores))
(println "  found 95 at: " (position 95 exam-scores))
(println "  count of 95: " (count 95 exam-scores))
(println "  without 95:  " (remove 95 exam-scores))

; 用字符串函数 + 序列函数处理文本数据
(define raw-names '("  Alice  " "BOB" "  Charlie" "DAVE  "))
(println "  raw names:   " raw-names)
(println "  cleaned:     " (map (lambda (n) (string-upcase (string-trim n))) raw-names))
(println "  find 'BOB':  " (find "BOB" raw-names))
(println "  count 'BOB': " (count "BOB" raw-names))

(println)
(println "===== Lisp 兼容语法演示完毕 =====")
