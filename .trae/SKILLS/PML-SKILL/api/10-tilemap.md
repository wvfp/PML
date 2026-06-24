# 10 — Tilemap System

## 概述

Tilemap 系统用于构建和渲染瓦片地图，支持正交和等距投影、多层叠放。

**管线**: `define-tileset` → `make-tilemap` → `tilemap-set!` → `render-tilemap`

---

## define-tileset — 定义瓦片集

```scheme
(define-tileset name
  :tile-size N
  :tiles '((id name graphic-expr [detail-expr])
           ...))
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `name` | symbol/string | 瓦片集名称（必填，第一个位置参数） |
| `:tile-size` | int | 瓦片像素尺寸（必填 kwarg） |
| `:tiles` | list | 瓦片类型定义列表（必填 kwarg） |

每个瓦片定义：
| 项 | 说明 |
|---|------|
| `id` | int，瓦片编号（0 保留给空瓦片） |
| `name` | symbol，瓦片名称 |
| `graphic-expr` | 图形表达式，求值为 GraphicObject |
| `detail-expr` | 可选覆盖层图形 |

**图形在定义时求值**，所有同一 tile-id 的实例渲染相同。

```scheme
(define-tileset 'terrain :tile-size 32
  :tiles '((1 grass  (rect 0 0 32 32 :fill "#2ecc71"))
           (2 stone  (rect 0 0 32 32 :fill "#95a5a6"))
           (3 water  (rect 0 0 32 32 :fill "#3498db"))
           (4 sand   (rect 0 0 32 32 :fill "#f1c40f"))))
```

---

## make-tilemap — 创建瓦片地图

```scheme
(make-tilemap tileset-name cols rows
  [:projection 'orthogonal|'isometric]
  [:layers N])
```

| 参数 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `tileset-name` | symbol/string | — | 已定义的瓦片集名称（必填） |
| `cols` | int | — | 列数（必填） |
| `rows` | int | — | 行数（必填） |
| `:projection` | symbol | `'orthogonal` | 投影方式 |
| `:layers` | int | 2 | 层数（0=ground, 1=decoration, ...） |

返回 tilemap 名称（可用作后续 `tilemap-set!` 和 `render-tilemap` 的参数）。

```scheme
(define tm (make-tilemap 'terrain 10 8 :layers 3))
```

---

## tilemap-set! — 设置瓦片

```scheme
(tilemap-set! tilemap-name layer col row tile-id)
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `tilemap-name` | symbol | tilemap 名称 |
| `layer` | int | 层索引（0-based） |
| `col` | int | 列（0-based） |
| `row` | int | 行（0-based） |
| `tile-id` | int | 瓦片类型 id（0=空） |

```scheme
; 第 0 层（地面）：铺草地
(for-each (lambda (c)
  (for-each (lambda (r)
    (tilemap-set! tm 0 c r 1))
    (range 8)))
  (range 10))

; 第 1 层：放一些石头
(tilemap-set! tm 1 3 2 2)
(tilemap-set! tm 1 7 5 2)
```

---

## render-tilemap — 渲染输出

```scheme
(render-tilemap tilemap-name
  [:output "path.png"]
  [:projection 'orthogonal|'isometric]
  [:layer N|:layers 'all])
```

| 参数 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `tilemap-name` | symbol/string | — | tilemap 名称（必填） |
| `:output` | string | — | 输出文件路径（必填 kwarg） |
| `:projection` | symbol | 同 tilemap 设置 | 可选覆盖 |
| `:layer` / `:layers` | int / symbol | `'all` | 指定层或全部 |

```scheme
; 渲染全部层
(render-tilemap tm :output "full_map.png")

; 只渲染第 0 层
(render-tilemap tm :output "ground_layer.png" :layer 0)
```

---

## 投影模式

### 正交 (Orthogonal)

默认模式。瓦片按网格直接排列，每个瓦片占据 `tile_size × tile_size` 像素。

```
输出尺寸: cols×tile_size × rows×tile_size
```

渲染方式：内部使用 Skia `drawAtlas` 批量绘制。

### 等距 (Isometric)

菱形排列。瓦片在水平方向偏移半个瓦片宽度。

```
输出尺寸: (cols+rows)×tile_size/2 × (cols+rows)×tile_size/4
渲染方式: Painter's Algorithm（从后往前绘制）
```

---

## 完整示例

```scheme
; 1. 定义瓦片集
(define-tileset 'world :tile-size 32
  :tiles '((1 grass  (rect 0 0 32 32 :fill "#2ecc71"))
           (2 stone  (rect 0 0 32 32 :fill "#95a5a6"))
           (3 water  (rect 0 0 32 32 :fill "#3498db"))
           (4 flower (group
                       (rect 12 18 8 14 :fill "#27ae60")
                       (circle 16 12 6 :fill "#e74c3c")))))

; 2. 创建地图
(define tm (make-tilemap 'world 8 6 :layers 2))

; 3. 填充地图（第 0 层 = 地面）
(for-each (lambda (c)
  (for-each (lambda (r) (tilemap-set! tm 0 c r 1)) (range 6)))
  (range 8))

; 4. 添加装饰（第 1 层）
(tilemap-set! tm 1 2 1 4)  ; flower
(tilemap-set! tm 1 5 3 4)  ; flower
(tilemap-set! tm 1 0 0 2)  ; stone
(tilemap-set! tm 1 7 5 2)  ; stone

; 5. 渲染输出
(render-tilemap tm :output "world.png")
```

---

## 注意事项

- **tile-id = 0** 保留表示"空"，始终渲染为透明
- **图形在 define-tileset 时求值**，所有同 id 实例共享相同渲染
- **输出不受 CLI `-o` 标记影响**，`render-tilemap` 直接输出到 `:output` 指定的路径
- 多层的 depth 排序：layer 0 在最底，layer N 在最顶
- 等距模式使用 Painter's Algorithm：按 `(row + col)` 排序从后向前绘制
