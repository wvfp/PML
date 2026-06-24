# 15 — Layer / Composition System

## 概述

层和合成系统用于构建复杂的多层项目结构，支持独立的层变换和整体输出。

---

## Layer

| 函数 | 签名 | 说明 |
|------|------|------|
| `make-layer` | `(make-layer content [:name name] [:transform m] [:opacity a])` | 创建层 |
| `layer?` | `(layer? v)` | 判断是否为层 |

```scheme
(define bg-layer (make-layer
                   (rect 0 0 800 600 :fill "#2c3e50")
                   :name "background"))
(define fg-layer (make-layer
                   (circle 400 300 100 :fill "#e74c3c")
                   :name "foreground"))
```

---

## Group（Layer 组）

| 函数 | 签名 | 说明 |
|------|------|------|
| `make-group` | `(make-group layers [:name name])` | 组合多个层为一个组 |

```scheme
(define scene-group (make-group
                      (list bg-layer fg-layer)
                      :name "scene"))
```

---

## Composition

| 函数 | 签名 | 说明 |
|------|------|------|
| `make-composition` | `(make-composition [:width W] [:height H] [:bg color] [:fps N])` | 创建合成 |
| `composition?` | `(composition? v)` | 判断是否为合成 |
| `composition-add` | `(composition-add comp layer-or-group)` | 向合成添加层/组 |
| `composition-render` | `(composition-render comp :output path)` | 渲染合成到文件 |

```scheme
(define comp (make-composition :width 800 :height 600 :bg "#ecf0f1"))
(composition-add comp bg-layer)
(composition-add comp fg-layer)
(composition-render comp :output "composition.png")
```

---

## Project

| 函数 | 签名 | 说明 |
|------|------|------|
| `project-render-all` | `(project-render-all comp [:output-dir dir])` | 渲染合成中所有可见层为独立文件 |

```scheme
; 批量项目导出
(project-render-all comp :output-dir "render_output/")
; → render_output/background.png
; → render_output/foreground.png
; → render_output/composition.png (合成图)
```

---

## Layer 变换

层支持独立的仿射变换：

```scheme
(define animated-layer
  (make-layer
    (circle 50 50 30 :fill "#3498db")
    :name "ball"
    :transform (translate 100 0)
    :opacity 0.8))
```

---

## 完整管线

```scheme
; 1. 创建内容层
(define bg  (make-layer (rect 0 0 800 600 :fill "#1a1a2e") :name "bg"))
(define fg  (make-layer (rect 50 50 700 500 :fill "#16213e") :name "fg"))
(define obj (make-layer (circle 400 300 80 :fill "#e94560")
                         :name "object"
                         :transform (translate -50 30)))

; 2. 创建合成
(define comp (make-composition :width 800 :height 600))
(composition-add comp bg)
(composition-add comp fg)
(composition-add comp obj)

; 3. 渲染
(composition-render comp :output "final.png")
```

---

## 注意事项

- Layer 的顺序影响渲染次序（先添加先渲染）
- `:transform` 为可选的 `AffineTransform` 值
- `:opacity` 范围为 0-1
- Composition 的宽高影响所有子层的裁剪区域
- `project-render-all` 的独立层输出自动添加层名后缀
