# 06 — Transforms (变换 + 矩阵)

## 变换矩阵 builtins

所有变换 builtin 创建并返回 `AffineTransform` 值。

### 基本变换

| 函数 | 签名 | 说明 |
|------|------|------|
| `translate` | `(translate tx ty)` | 平移矩阵 |
| `rotate` | `(rotate angle [cx cy])` | 旋转（弧度），可选绕中心 |
| `scale` | `(scale sx [sy])` | 缩放，sy 省略则等同 sx |
| `shear` | `(shear shx shy)` | 切变 |
| `identity-matrix` | `(identity-matrix)` | 单位矩阵 |

### 矩阵运算

| 函数 | 签名 | 说明 |
|------|------|------|
| `compose` | `(compose m1 m2 ...)` | 矩阵累乘（组合变换） |
| `matrix-inverse` | `(matrix-inverse m)` | 矩阵求逆 |
| `matrix-apply` | `(matrix-apply m x y)` | 矩阵作用于点 |
| `matrix?` | `(matrix? v)` | 判断是否是变换矩阵 |

### 示例

```scheme
(define m (compose (translate 50 50) (rotate 0.5)))
(matrix-apply m 10 0)  → (62.2, 59.3)  ; 近似值
```

---

## 对象级变换

这些 builtin 将变换应用到 GraphicObject 本身（而非环境矩阵）。

| 函数 | 签名 | 说明 |
|------|------|------|
| `with-transform` | `(with-transform go m)` | 用变换矩阵 m 变换 GraphicObject go |
| `translate-object` | `(translate-object go dx dy)` | 平移对象 |
| `rotate-object` | `(rotate-object go angle [cx cy])` | 旋转对象（弧度），可选绕中心 |
| `scale-object` | `(scale-object go sx [sy])` | 缩放对象 |

### 示例

```scheme
; 创建并旋转矩形
(define box (rect 0 0 40 40 :fill "red"))
(define rotated (rotate-object box 0.785))  ; 45度旋转
(render rotated "rotated.png")

; 组合变换
(arrow (translate-object
         (rotate-object (rect 0 0 30 10 :fill "blue") 1.57)
         50 50))
```

---

## 变换顺序

`compose` 中的顺序与数学约定一致：`(compose A B)` 表示"先 A 后 B"。

```scheme
; 以下效果相同：
(translate-object (rotate-object box 0.5) 100 0)
(with-transform box (compose (rotate 0.5) (translate 100 0)))
; 都先旋转 0.5 弧度，再平移 (100, 0)
```

---

## 实用模式

```scheme
; 原地居中缩放
(define (scale-from-center obj sx sy)
  (let ((cx (/ (graphic-width obj) 2))
        (cy (/ (graphic-height obj) 2)))
    (with-transform obj
      (compose (translate cx cy)
               (scale sx sy)
               (translate (- cx) (- cy))))))

; 多次旋转动画
(define (rotate-steps obj steps)
  (let ((angle (/ (* 2 3.14159) steps)))
    (map (lambda (i) (rotate-object obj (* angle i)))
         (range steps))))
```
