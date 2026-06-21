# 05 — 2D Graphics: Shapes, Style, Color

## 形状

所有形状 builtin 都返回一个 **GraphicObject**（不可变值）。使用 kwargs 传递样式参数。

### 基本形状

| 函数 | 签名 | 说明 |
|------|------|------|
| `circle` | `(circle cx cy r [:fill] [:stroke] [:stroke-width])` | 圆 |
| `rect` | `(rect x y w h [:fill] [:stroke] [:stroke-width] [:rx])` | 矩形，:rx=圆角 |
| `ellipse` | `(ellipse cx cy rx ry [:fill] [:stroke] [:stroke-width])` | 椭圆 |
| `line` | `(line x1 y1 x2 y2 [:stroke] [:stroke-width])` | 线段 |
| `polygon` | `(polygon x1 y1 x2 y2 ... [:fill] [:stroke] [:stroke-width])` | 多边形 |
| `text` | `(text str x y [:fill] [:font-size])` | 文本（仅 ASCII/Latin，CJK 可能显示为 tofu） |

### group

组合多个 GraphicObject 为一个整体。

```scheme
(group (rect 0 0 100 100 :fill "red")
       (circle 50 50 30 :fill "blue"))
```

### 样式参数

所有形状共享的可选 kwargs：

| 参数 | 类型 | 说明 |
|------|------|------|
| `:fill` | string | 填充颜色，格式见下方 |
| `:stroke` | string | 描边颜色 |
| `:stroke-width` | number | 描边宽度（默认 0） |
| `:opacity` | number | 不透明度 0-1（部分形状支持） |

---

## 颜色

### 颜色格式

PML 颜色统一使用字符串指定：

| 格式 | 示例 |
|------|------|
| `#RRGGBB` | `"#e74c3c"` |
| `#RRGGBBAA` | `"#e74c3c80"` |
| 命名颜色 | `"red"`, `"blue"`, `"green"`, `"black"`, `"white"`, `"transparent"` |
| CSS 命名 | 支持 140+ CSS 颜色名 |

### 颜色 builtin

| 函数 | 签名 | 说明 |
|------|------|------|
| `color/rgb` | `(color/rgb r g b)` | 创建颜色字符串 `#RRGGBB`，r/g/b 为 0-255 int |
| `color/rgba` | `(color/rgba r g b a)` | 同上含 alpha，a 为 0-255 int |

```scheme
(color/rgb 231 76 60)    → "#e74c3c"
(color/rgba 231 76 60 128) → "#e74c3c80"
```

---

## 对象级样式

这些 builtin 应用于单个 GraphicObject，与全局样式不同。

| 函数 | 签名 | 说明 |
|------|------|------|
| `fill` | `(fill graphic color)` | 设置/覆盖填充颜色，返回新 GraphicObject |
| `stroke` | `(stroke graphic color)` | 设置/覆盖描边颜色 |
| `no-fill` | `(no-fill graphic)` | 移除填充 |
| `no-stroke` | `(no-stroke graphic)` | 移除描边 |
| `stroke-width` | `(stroke-width graphic w)` | 设置描边宽度 |

```scheme
(no-fill (circle 50 50 30 :stroke "red" :stroke-width 3))
; → 一个空心红圆
```

---

## 常用模式

```scheme
; 带边框的圆角矩形
(rect 10 10 100 60 :fill "#34495e" :stroke "#2c3e50" :stroke-width 2 :rx 8)

; 饼图扇形（用 polygon 近似）
(polygon 50 50 50 20 62 30 :fill "#e74c3c" :stroke "black")

; 多层组合
(define icon
  (group
    (circle 32 32 28 :fill "#3498db")
    (circle 32 32 20 :fill "#2980b9")
    (circle 32 32 12 :fill "#1abc9c")))

; 文本居中（Skia 基准线在左下）
(define label
  (group
    (rect 0 0 120 40 :fill "#2c3e50" :rx 6)
    (text "Hello" 20 26 :fill "white" :font-size 16)))
```
