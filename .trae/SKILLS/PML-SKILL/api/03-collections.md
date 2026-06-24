# 03 — Collections (列表 / 向量 / 哈希 / 字符串)

## 向量

### 创建与基础操作

| 函数 | 签名 | 说明 |
|------|------|------|
| `make-vector` | `(make-vector size [init])` | 创指定大小向量，可选填充值 |
| `vector?` | `(vector? v)` | 判断是否是向量 |
| `vector-length` | `(vector-length v)` | 向量长度 |
| `vector-ref` | `(vector-ref v index)` | 读取元素 |
| `vector-set!` | `(vector-set! v index val)` | 写入元素 |
| `vector-copy` | `(vector-copy v)` | 浅拷贝向量 |
| `vector-fill!` | `(vector-fill! v val)` | 填充所有元素 |
| `vector->list` | `(vector->list v)` | 向量转列表 |
| `list->vector` | `(list->vector lst)` | 列表转向量 |

### 示例

```scheme
(define v (make-vector 5 0))        → #(0 0 0 0 0)
(vector-set! v 2 42)                → #(0 0 42 0 0)
(vector-ref v 2)                    → 42
(vector->list v)                    → (0 0 42 0 0)
```

---

## 哈希表

| 函数 | 签名 | 说明 |
|------|------|------|
| `make-hash` | `(make-hash)` | 创建空哈希表 |
| `hash?` | `(hash? v)` | 判断是否是哈希表 |
| `hash-ref` | `(hash-ref ht key [default])` | 读取键值，可选默认值 |
| `hash-set!` | `(hash-set! ht key val)` | 写入键值对 |
| `hash-delete!` | `(hash-delete! ht key)` | 删除键 |
| `hash-keys` | `(hash-keys ht)` | 返回所有键的列表 |
| `hash-values` | `(hash-values ht)` | 返回所有值的列表 |

### 示例

```scheme
(define h (make-hash))
(hash-set! h "name" "Alice")
(hash-set! h "age" 30)
(hash-ref h "name")           → "Alice"
(hash-ref h "missing" "n/a")  → "n/a"
(hash-keys h)                 → ("name" "age")
```

---

## 字符串

### 操作

| 函数 | 签名 | 说明 |
|------|------|------|
| `string?` | `(string? v)` | 判断是否是字符串 |
| `string-append` | `(string-append s ...)` | 拼接字符串 |
| `string-length` | `(string-length s)` | 字符串长度（UTF-8 字节数） |
| `string-ref` | `(string-ref s index)` | 取指定索引的字符编码 |
| `substring` | `(substring s start end)` | 取子串 [start, end) |
| `string-copy` | `(string-copy s)` | 拷贝字符串 |
| `make-string` | `(make-string k [char])` | 创建指定长度的字符串 |
| `number->string` | `(number->string n)` | 数字转字符串 |
| `string->number` | `(string->number s)` | 字符串转数字 |
| `symbol->string` | `(symbol->string s)` | 符号转字符串 |
| `string->symbol` | `(string->symbol s)` | 字符串转符号 |

### 比较

| 函数 | 签名 | 说明 |
|------|------|------|
| `string=?` | `(string=? a b)` | 字符串相等 |
| `string<?` | `(string<? a b)` | 小于（字典序） |
| `string<=?` | `(string<=? a b)` | 小于等于 |
| `string>?` | `(string>? a b)` | 大于 |
| `string>=?` | `(string>=? a b)` | 大于等于 |

### 示例

```scheme
(string-append "Hello, " "World!")  → "Hello, World!"
(substring "abcdef" 1 4)            → "bcd"
(string-ref "ABC" 1)                → 66  ; 'B' 的编码
```
