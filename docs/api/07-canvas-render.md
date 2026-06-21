# 07 — Canvas Pipeline + Render

## Canvas 系统

Canvas 是一个累加器：将多次 `add` 的 GraphicObject 收集起来，在 `render` 时一次性输出。

### Canvas builtins

| 函数 | 签名 | 说明 |
|------|------|------|
| `canvas` | `(canvas [:bg color])` | 创建新 Canvas |
| `sprite-canvas` | `(sprite-canvas [:bg] [:anchor] [:padding])` | 创建 Sprite Canvas（带裁剪边界） |
| `clear-canvas` | `(clear-canvas c)` | 清空 Canvas，移除所有已添加对象 |
| `add` | `(add canvas graphic)` | 添加 GraphicObject 到 Canvas |

### 示例

```scheme
; 基本用法
(define c (canvas :bg "#ecf0f1"))
(add c (rect 10 10 80 80 :fill "#3498db"))
(add c (circle 50 50 30 :fill "#e74c3c" :opacity 0.8))
(render c "output.png")
```

---

## Render 系统

### render — 渲染输出

```scheme
; 直接渲染 GraphicObject：
(render graphic filename [:format fmt] [:fps N] [:duration N])

; 渲染 Canvas（包含所有 add 的对象）：
(render canvas filename [:format fmt])

; 仅指定文件名（将 canvas 内容保存到文件）：
(render "output.png")
```

**参数**:
| 参数 | 类型 | 说明 |
|------|------|------|
| `:format` | string | 输出格式 (`"PNG"` / `"GIF"`)，默认根据扩展名推断 |
| `:fps` | int | GIF 帧率（动画使用） |
| `:duration` | number | 动画总时长（秒） |

### render-set — 多尺寸渲染

批量生成同一 Sprite 的多个缩放版本（适用于游戏素材）。

```scheme
(render-set name
  :content graphic
  [:scales '(1 2 4)]
  [:base-size '(64 64)]
  [:format "PNG"])
```

**参数**:
| 参数 | 类型 | 说明 |
|------|------|------|
| `:content` | GraphicObject | 要渲染的图形（必填） |
| `:scales` | list | 缩放倍数列表，默认 `(1 2 4)` |
| `:base-size` | list | 基本尺寸 `(w h)`，默认 `(64 64)` |
| `:format` | string | 输出格式 |

**输出**: 每个 scale 生成一个文件，如 `name_1x.png`, `name_2x.png`, `name_4x.png`

### render-spritesheet — Sprite Sheet 生成

```scheme
(render-spritesheet
  :sprites '((sprite1 . obj1) (sprite2 . obj2))
  [:sheet-width 256] [:sheet-height 256]
  [:columns 4]
  [:output "spritesheet.png"])
```

将多个 GraphicObject 排列到一张大图上。

---

## 工作流模式

### 单次渲染（直接渲染 GraphicObject）

```scheme
(render
  (rect 0 0 100 100 :fill "#2ecc71")
  "output.png")
```

### Canvas 管线（组合多个对象）

```scheme
(let ((c (canvas :bg "#2c3e50")))
  (add c (rect 20 20 60 60 :fill "#e74c3c"))
  (add c (circle 50 50 20 :fill "#f1c40f"))
  (render c "composite.png"))
```

### 游戏素材批处理

```scheme
(define hero (group ...))
(render-set "hero"
  :content hero
  :scales '(1 2 4)
  :base-size '(64 64))
; → hero_1x.png, hero_2x.png, hero_4x.png
```

### 关键区别

| | render 单对象 | render Canvas | render-set |
|--|--------------|---------------|------------|
| 输入 | 单个 GraphicObject | Canvas | GraphicObject + scale 列表 |
| 输出 | 一个文件 | 一个文件 | 多个文件（1x/2x/4x） |
| 适用 | 简单场景 | 多层合成 | 游戏素材批处理 |
