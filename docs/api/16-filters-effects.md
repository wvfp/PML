# 16 — Filter / Effects

## 概述

滤镜链系统，支持组合多个滤镜效果顺序应用到 GraphicObject。

---

## 滤镜类型

| 函数 | 签名 | 说明 |
|------|------|------|
| `color-adjust` | `(color-adjust [:brightness N] [:contrast N] [:saturation N] [:hue N])` | 颜色调整 |
| `levels` | `(levels [:in-black N] [:in-white N] [:out-black N] [:out-white N] [:gamma N])` | 色阶调整 |
| `curves` | `(curves :channel 'rgb\|'red\|'green\|'blue :points '((x1 y1) (x2 y2) ...))` | 曲线调整 |
| `threshold` | `(threshold [:level N] [:mode 'binary\|'linear])` | 阈值 |
| `posterize` | `(posterize [:levels N])` | 色调分离 |
| `blur` | `(blur [:radius N])` | 高斯模糊 |
| `sharpen` | `(sharpen [:amount N])` | 锐化 |
| `edge-detect` | `(edge-detect)` | 边缘检测 |
| `emboss` | `(emboss [:amount N])` | 浮雕 |
| `convolution` | `(convolution :kernel '(k11 k12 ...) :divisor N :bias N)` | 自定义卷积核 |
| `drop-shadow` | `(drop-shadow [:offset '(x y)] [:radius N] [:color c])` | 投影阴影 |
| `inner-shadow` | `(inner-shadow [:offset '(x y)] [:radius N] [:color c])` | 内阴影 |
| `outer-glow` | `(outer-glow [:radius N] [:color c])` | 外发光 |
| `inner-glow` | `(inner-glow [:radius N] [:color c])` | 内发光 |
| `bevel-emboss` | `(bevel-emboss [:depth N] [:direction 'up\|'down])` | 斜面和浮雕 |

---

## filter-chain — 滤镜链

组合多个滤镜，顺序应用。

```scheme
(filter-chain filter1 filter2 ...)
```

```scheme
; 定义阴影 + 模糊的滤镜链
(define photo-effect (filter-chain
  (drop-shadow :offset '(3 3) :radius 5 :color "rgba(0,0,0,0.5)")
  (blur :radius 1)))

; 应用到图形
(define panel (group
  (rect 0 0 120 80 :fill "#ecf0f1")
  (apply-filter photo-effect (circle 60 40 25 :fill "#e74c3c"))))
```

---

## apply-filter

```scheme
(apply-filter filter graphic)
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `filter` | filter 或 filter-chain | 要应用的滤镜 |
| `graphic` | GraphicObject | 目标图形 |

```scheme
; 滤镜链：发光按钮
(define glow (filter-chain
  (outer-glow :radius 8 :color "rgba(46,204,113,0.6)")
  (drop-shadow :offset '(2 2) :radius 4 :color "rgba(0,0,0,0.3)")))

(define btn (apply-filter glow
  (rect 0 0 100 40 :fill "#2ecc71" :rx 6)))
```

---

## filter? — 类型判断

```scheme
(filter? v)  → #t 如果是滤镜或滤镜链
```

---

## 滤镜模式

```scheme
; UI 投影
(define ui-shadow (drop-shadow :offset '(1 1) :radius 2 :color "#rgba(0,0,0,0.5)"))

; 发光效果
(define glow-effect (outer-glow :radius 10 :color "#3498db"))

; 素描效果
(define sketch (filter-chain
  (edge-detect)
  (threshold :level 0.5)))

; 复古滤镜
(define vintage (filter-chain
  (color-adjust :saturation 0.6 :contrast 1.2)
  (curves :channel 'rgb :points '((0 0) (64 48) (192 208) (255 255)))
  (blur :radius 0.5)))
```

---

## 注意事项

- 滤镜链的顺序影响效果：后面的滤镜作用于前面滤镜的输出
- Skia 的 `SkImageFilter` 实现，有些滤镜组合可能因内部实现限制效果不同
- 卷积核使用一维列表，按行主序排列
- `curves` 的点列表至少 2 个，最多 20 个，需按 x 升序排列
- `color-adjust` 的 hue 用角度制（0-360），saturation/contrast/brightness 为 0.0-2.0
