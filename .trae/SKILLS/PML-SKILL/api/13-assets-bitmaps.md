# 13 — Asset / Bitmap I/O

## 概述

位图加载、Spritesheet 解析和缓存管理。

---

## 图像加载

| 函数 | 签名 | 说明 |
|------|------|------|
| `image` | `(image path x y [:w W] [:h H] [:crop rect])` | 从文件加载图像为 GraphicObject |
| `load-image` | `(load-image path)` | 加载图像到缓存，返回图像对象 |
| `asset-path?` | `(asset-path? path)` | 检查资源路径是否存在 |
| `clear-assets!` | `(clear-assets!)` | 清空资源缓存 |

### image

```scheme
(image "sprites/hero.png" 0 0)                    ; 原始尺寸
(image "sprites/hero.png" 0 0 :w 64 :h 64)        ; 缩放
(image "sprites/hero.png" 0 0 :crop '(10 10 40 40)) ; 裁剪
```

**参数**:
| 参数 | 类型 | 说明 |
|------|------|------|
| `path` | string | 文件路径（必填，位置参数） |
| `x`, `y` | number | 位置（必填） |
| `:w`, `:h` | number | 可选缩放尺寸 |
| `:crop` | list | 可选裁剪 `(x y w h)` |

---

## Spritesheet

| 函数 | 签名 | 说明 |
|------|------|------|
| `load-spritesheet` | `(load-spritesheet path :frame-w N :frame-h N [:cols N] [:rows N] [:margin N])` | 加载 spritesheet 为 GraphicsList |

```scheme
; 加载 4×4 的 spritesheet，每帧 32x32
(define sprites (load-spritesheet "hero-sheet.png"
                   :frame-w 32 :frame-h 32 :cols 4 :rows 4))
(define walk1 (list-ref sprites 0))
(define walk2 (list-ref sprites 1))
```

**参数**:
| 参数 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `path` | string | — | spritesheet 文件路径 |
| `:frame-w` | int | — | 每帧宽度 |
| `:frame-h` | int | — | 每帧高度 |
| `:cols` | int | 1 | 列数 |
| `:rows` | int | 1 | 行数 |
| `:margin` | int | 0 | 帧间距 |

---

## Bitmap Layer

| 函数 | 签名 | 说明 |
|------|------|------|
| `bitmap-layer` | `(bitmap-layer graphic [:w W] [:h H] [:name name])` | 将 GraphicObject 渲染为位图层 |

将矢量图形栅格化为位图层（Layer 对象），用 Composition 合成。

---

## 资源管理

- 位图通过 AssetCache 缓存（源文件路径 → SkImage）
- `clear-assets!` 清空缓存，释放所有图像内存
- 路径相对于执行时 CWD 或 `__source_file__` 所在目录解析

```scheme
(if (asset-path? "sprites/hero.png")
    (image "sprites/hero.png" 0 0)
    (rect 0 0 32 32 :fill "red"))  ; fallback
```
