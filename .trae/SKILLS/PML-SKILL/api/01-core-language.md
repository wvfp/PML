# 01 — Core Language (特殊形式 + 核心函数)

## 特殊形式

特殊形式在求值器层面硬编码，参数不一定全部求值。

### define

绑定值到当前环境的符号。

```scheme
(define pi 3.14159)
(define name (circle 50 50 30 :fill "red"))
```

### lambda

创建闭包函数。

```scheme
(define square (lambda (x) (* x x)))
(define add (lambda (a b) (+ a b)))
(define greet (lambda (name) (string-append "Hello, " name)))
```

### if / cond / and / or

```scheme
(if (> x 0) "positive" "non-positive")
(cond ((> x 0) "positive")
      ((< x 0) "negative")
      (else "zero"))
(and (number? x) (> x 0))
(or (eq? color "red") (eq? color "blue"))
```

### begin

顺序求值多个表达式，返回最后一个的值。

```scheme
(begin
  (define x 42)
  (define y (* x 2))
  (+ x y))
```

### let / let\*

局部绑定。

```scheme
; let — 并行求值
(let ((x 1) (y 2)) (+ x y))

; let* — 串行求值（后面的可见前面的）
(let* ((x 1) (y (+ x 1))) (- y x))
```

### set!

修改变量（按词法作用域查找）。

```scheme
(define counter 0)
(set! counter (+ counter 1))
```

### do

迭代循环。

```scheme
(do ((i 0 (+ i 1)))
    ((= i 10) 'done)
  (println i))
```

### quote / quasiquote

```scheme
(quote (1 2 3))     → (1 2 3)
'(1 2 3)            → (1 2 3)         ; 简写
`(1 ,(+ 1 2) 4)     → (1 3 4)         ; quasiquote
```

### define-macro / defmacro

```scheme
(define-macro (twice expr) `(begin ,expr ,expr))
(defmacro (unless cond body) `(if (not ,cond) ,body))
```

### macroexpand

调试宏展开。

```scheme
(macroexpand '(unless (< x 0) "positive"))
```

### assert

```scheme
(assert (= (+ 1 2) 3))
(assert (> x 0) "x must be positive")
```

### gensym

生成唯一符号。

```scheme
(gensym)  → g1
(gensym)  → g2
```

---

## 核心函数

### 类型检测

| 函数 | 签名 | 说明 |
|------|------|------|
| `number?` | `(number? v)` | 是数字（int 或 float）？ |
| `integer?` | `(integer? v)` | 是整数？ |
| `float?` | `(float? v)` | 是浮点数？ |
| `string?` | `(string? v)` | 是字符串？ |
| `boolean?` | `(boolean? v)` | 是布尔值？ |
| `symbol?` | `(symbol? v)` | 是符号？ |
| `keyword?` | `(keyword? v)` | 是关键字？ |
| `null?` | `(null? v)` | 是 nil？ |
| `pair?` | `(pair? v)` | 是 pair（非空列表）？ |
| `list?` | `(list? v)` | 是列表？ |
| `procedure?` | `(procedure? v)` | 是过程（lambda/builtin）？ |
| `typeof` | `(typeof v)` | 返回值类型名称字符串 |

### 类型转换

| 函数 | 签名 | 说明 |
|------|------|------|
| `number->string` | `(number->string n)` | 数字转字符串 |
| `string->number` | `(string->number s)` | 字符串转数字，失败返回 #f |
| `string->symbol` | `(string->symbol s)` | 字符串转符号 |
| `symbol->string` | `(symbol->string s)` | 符号转字符串 |
| `list->string` | `(list->string chars)` | 字符编码列表转字符串 |
| `string->list` | `(string->list s)` | 字符串转字符编码列表 |

### 错误处理

| 函数 | 签名 | 说明 |
|------|------|------|
| `error` | `(error message)` | 触发运行时错误 |

---

## 列表核心操作

| 函数 | 签名 | 说明 |
|------|------|------|
| `cons` | `(cons a b)` | 构造 pair |
| `car` | `(car pair)` | 取第一个元素 |
| `cdr` | `(cdr pair)` | 取剩余部分 |
| `list` | `(list a b ...)` | 创建列表 |
| `length` | `(length list)` | 列表长度 |
| `append` | `(append list ...)` | 拼接列表 |
| `reverse` | `(reverse list)` | 反转列表 |
| `list-ref` | `(list-ref list index)` | 按索引取元素 |
| `list-tail` | `(list-tail list k)` | 取第 k 个元素开始的子列表 |
| `member` | `(member item list)` | 查找元素（用 equal?），返回子列表或 #f |
| `memq` | `(memq item list)` | 同 member 但用 eq? |
| `assoc` | `(assoc key alist)` | alist 按 key 查找（equal?） |
| `assq` | `(assq key alist)` | alist 按 key 查找（eq?） |
| `range` | `(range end)` / `(range start end [step])` | 生成整数列表 |
| `nth` | `(nth list index)` | 取第 index 个元素（0-based） |
| `sort` | `(sort list [cmp])` | 排序 |

### map / filter / reduce / for-each

```scheme
(map (lambda (x) (* x 2)) '(1 2 3))        → (2 4 6)
(filter (lambda (x) (> x 2)) '(1 2 3 4))   → (3 4)
(reduce + 0 '(1 2 3 4))                    → 10
(for-each println '(1 2 3))
```

### apply

```scheme
(apply + '(1 2 3))      → 6
(apply max '(5 2 8 1))  → 8
```

### format

```scheme
(format "Value: ~a, Count: ~d" "hello" 42)
```

### print / println

```scheme
(print "hello")     ; 打印不换行
(println "hello")   ; 打印换行
```
