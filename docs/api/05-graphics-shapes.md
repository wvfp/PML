# 05 — 2D Graphics: Shapes, Style, Color

## 形状

所有形状 builtin 都返回一个 **GraphicObject**（不可变值）。使用 kwargs 传递样式参数。

### 基本形状

| 函数 | 签名 | 说明 |
|------|------|------|
| `circle` | `(circle cx cy r [:fill] [:stroke] [:stroke-width] [:blend-mode] [:stroke-align])` | 圆 |
| `rect` | `(rect x y w h [:fill] [:stroke] [:stroke-width] [:rx] [:blend-mode] [:stroke-align])` | 矩形，:rx=圆角 |
| `ellipse` | `(ellipse cx cy rx ry [:fill] [:stroke] [:stroke-width] [:blend-mode] [:stroke-align])` | 椭圆 |
| `line` | `(line x1 y1 x2 y2 [:stroke] [:stroke-width] [:blend-mode])` | 线段（:stroke-align 不适用） |
| `polygon` | `(polygon pts [:fill] [:stroke] [:stroke-width] [:blend-mode] [:stroke-align])` | 多边形，pts = `(list (list x y) ...)` |
| `path` | `(path cmds [:d svg-str] [:fill] [:stroke] [:stroke-width] [:blend-mode] [:stroke-align])` | SVG 兼容路径，支持 S-expr 命令列表或 `:d` 字符串 |
| `text` | `(text str x y [:fill] [:font-size] [:blend-mode])` | 文本（:stroke-align 不适用） |

### group

组合多个 GraphicObject 为一个整体。

```scheme
(group (rect 0 0 100 100 :fill "red")
       (circle 50 50 30 :fill "blue"))
```

### path

创建 SVG 兼容的路径形状。支持两种输入格式：

**方式 1：S-expr 命令列表**

```scheme
(path (list (list 'move-to x0 y0)
            (list 'line-to x1 y1)
            (list 'cubic-to cx1 cy1 cx2 cy2 x2 y2)      
            (list 'close))
      :fill "#e74c3c" :stroke "#333" :stroke-width 2)   
```

**方式 2：SVG 路径字符串（`:d` 关键字）**

```scheme
(path :d "M10 10 L100 50 C150 0 200 100 250 50 Z"       
      :fill "#e74c3c" :stroke "#333")
```

两种方式同时提供时，`:d` 字符串优先。

#### 命令列表

每个命令是一个 list，首元素为命令符号，后续为参数：     

| 命令 | 参数 | 说明 |
|------|------|------|
| `move-to` | `x y` | 移动到绝对坐标 |
| `move-to-r` | `dx dy` | 相对移动到 |
| `line-to` | `x y` | 画线到绝对坐标 |
| `line-to-r` | `dx dy` | 相对画线到 |
| `hline-to` | `x` | 水平线到 x |
| `hline-to-r` | `dx` | 相对水平线 |
| `vline-to` | `y` | 垂直线到 y |
| `vline-to-r` | `dy` | 相对垂直线 |
| `cubic-to` | `cx1 cy1 cx2 cy2 x y` | 三次贝塞尔曲线 | 
| `cubic-to-r` | `dcx1 dcy1 dcx2 dcy2 dx dy` | 相对三次 贝塞尔 |
| `smooth-cubic-to` | `cx2 cy2 x y` | 平滑三次（自动反射控制点） |
| `smooth-cubic-to-r` | `dcx2 dcy2 dx dy` | 相对平滑三次 |
| `quad-to` | `cx cy x y` | 二次贝塞尔曲线 |
| `quad-to-r` | `dcx dcy dx dy` | 相对二次贝塞尔 |      
| `smooth-quad-to` | `x y` | 平滑二次（自动反射控制点） |
| `smooth-quad-to-r` | `dx dy` | 相对平滑二次 |
| `arc` | `rx ry rot laf sf x y` | 椭圆弧段 |
| `arc-r` | `rx ry rot laf sf dx dy` | 相对椭圆弧段 |   
| `close` | — | 闭合当前子路径 |

**arc 参数详解：**
- `rx`, `ry` — 椭圆半轴半径
- `rot` — x 轴旋转角度（度）
- `laf` — 大弧标志：`0` 小弧，`1` 大弧
- `sf` — 扫掠方向：`0` 逆时针，`1` 顺时针

#### SVG 路径字符串

遵循 SVG 2.0 Path 规范，支持所有标准命令字母：

```
M = move-to    L = line-to    H = horizontal line       
V = vertical line    C = cubic    S = smooth cubic      
Q = quad    T = smooth quad    A = arc    Z = close     
```

大写=绝对坐标，小写=相对坐标。支持隐式重复命令、逗号/空格分隔、科学计数法。

```scheme
; 心形曲线
(path :d "M 150 100 C 150 60, 200 40, 250 100 C 300 160, 400 120, 400 100"
      :stroke "#e74c3c" :stroke-width 3 :fill "none")   

; 齿轮状多边形
(path :d "M 50 50 L 150 50 L 150 150 L 50 150 Z"        
      :fill "#3498db" :stroke "#2c3e50" :stroke-width 2)
```

#### 手绘风格

`path` 也支持 rough-style 渲染：

```scheme
(path (list (list 'move-to 10 10) (list 'line-to 100 50) (list 'close))
      :fill "#e74c3c" :roughness 2 :bowing 1)
```

### 样式参数

所有形状共享的可选 kwargs：

| 参数 | 类型 | 说明 |
|------|------|------|
| `:fill` | string | 填充颜色，格式见下方 |
| `:stroke` | string | 描边颜色 |
| `:stroke-width` | number | 描边宽度（默认 0） |       
| `:opacity` | number | 不透明度 0-1（部分形状支持） |  
| `:blend-mode` | string | 混合模式，覆盖 layer 级 blend（见 ❐ 19-混合与描边） |
| `:stroke-align` | string | 描边对齐：`"center"`（默认）\| `"inside"` \| `"outside"` |

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

; SVG 路径绘制心形
(add (path :d "M 90 130 C 90 90, 130 60, 160 90 C 190 60, 230 90, 230 130 C 230 180, 160 230, 160 230 C 160 230, 90 180, 90 130 Z"
          :fill "#FF6B6B" :stroke "#C92A2A" :stroke-width 2))

; 手绘风格路径
(add (path :d "M 50 100 C 80 50, 120 150, 150 100"
          :fill "none" :stroke "#845EF7" :stroke-width 3 :roughness 2))
```

---

## Edge Perturbation & Corner Rounding

`polygon` 支持边缘顶点扰动和圆角功能。详见 [20-edge-perturb.md](20-edge-perturb.md)。

```scheme
; 带扰动的多边形
(add (polygon '((0 0) (100 0) (100 100) (0 100))
              :edge-noise 0.15
              :edge-subdiv 3
              :edge-seed 42
              :fill "#e74c3c"))

; 圆角多边形
(add (polygon '((0 0) (100 0) (100 100) (0 100))
              :corner-radius 8.0
              :edge-subdiv 1
              :fill "#3498db"))
```
