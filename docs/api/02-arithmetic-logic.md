# 02 — Arithmetic / Comparison / Predicates

## 算术运算

所有算术函数接受 int 或 float 混合，返回 float 若任一参数是 float，否则返回 int。

### 基本运算

| 函数 | 签名 | 说明 |
|------|------|------|
| `+` | `(+ a b ...)` | 加法 |
| `-` | `(- a b ...)` | 减法（一元=取反） |
| `*` | `(* a b ...)` | 乘法 |
| `/` | `(/ a b ...)` | 除法 |
| `%` | `(% a b ...)` | 取余/取模 |
| `abs` | `(abs n)` | 绝对值 |

### 数学函数

| 函数 | 签名 | 说明 |
|------|------|------|
| `sin` | `(sin x)` | 正弦（弧度） |
| `cos` | `(cos x)` | 余弦（弧度） |
| `tan` | `(tan x)` | 正切（弧度） |
| `asin` | `(asin x)` | 反正弦 |
| `acos` | `(acos x)` | 反余弦 |
| `atan` | `(atan x)` | 反正切 |
| `atan2` | `(atan2 y x)` | 二参反正切 |
| `sqrt` | `(sqrt x)` | 平方根 |
| `exp` | `(exp x)` | 指数 e^x |
| `log` | `(log x)` | 自然对数 |
| `pow` | `(pow base exp)` | 幂运算 |
| `expt` | `(expt base exp)` | 整数幂 |
| `floor` | `(floor x)` | 向下取整 |
| `ceil` | `(ceil x)` | 向上取整 |
| `round` | `(round x)` | 四舍五入 |

### 数论

| 函数 | 签名 | 说明 |
|------|------|------|
| `abs` | `(abs n)` | 绝对值 |
| `quotient` | `(quotient a b)` | 整除商 |
| `remainder` | `(remainder a b)` | 余数（与 quotient 符号一致） |
| `modulo` | `(modulo a b)` | 模运算（结果非负） |
| `gcd` | `(gcd a b ...)` | 最大公约数 |
| `lcm` | `(lcm a b ...)` | 最小公倍数 |
| `max` | `(max a b ...)` | 最大值 |
| `min` | `(min a b ...)` | 最小值 |

### 谓词

| 函数 | 签名 | 说明 |
|------|------|------|
| `positive?` | `(positive? n)` | n > 0？ |
| `negative?` | `(negative? n)` | n < 0？ |
| `zero?` | `(zero? n)` | n = 0？ |
| `even?` | `(even? n)` | 偶数？ |
| `odd?` | `(odd? n)` | 奇数？ |
| `random` | `(random [limit])` | 随机数（无参=0-1 float，有参=0-limit int） |

---

## 比较运算

| 函数 | 签名 | 说明 |
|------|------|------|
| `=` | `(= a b ...)` | 数值相等 |
| `<` | `(< a b ...)` | 小于 |
| `>` | `(> a b ...)` | 大于 |
| `<=` | `(<= a b ...)` | 小于等于 |
| `>=` | `(>= a b ...)` | 大于等于 |
| `eq?` | `(eq? a b)` | 引用相等（同对象/符号） |
| `equal?` | `(equal? a b)` | 结构相等（值比较） |
| `not` | `(not x)` | 逻辑非 |

### 示例

```scheme
(= 5 5.0)          → #t       ; 数值相等
(eq? 'a 'a)        → #t       ; 符号引用相等
(equal? '(1 2) '(1 2)) → #t   ; 结构相等
```

---

## 常用模式

```scheme
; 截断到范围
(define (clamp v lo hi)
  (min (max v lo) hi))

; 线性插值
(define (lerp a b t)
  (+ a (* (- b a) t)))

; 弧度与角度转换
(define (deg->rad d) (* d (/ pi 180.0)))
(define (rad->deg r) (* r (/ 180.0 pi)))

; 检查范围重叠
(define (overlap? a1 a2 b1 b2)
  (not (or (< a2 b1) (< b2 a1))))
```
