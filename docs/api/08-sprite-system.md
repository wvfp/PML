# 08 — Sprite System (Style / Palette / Components)

## Style 样式系统

定义可复用的样式模板。

| 函数 | 签名 | 说明 |
|------|------|------|
| `define-style` | `(define-style name [:fill] [:stroke] [:stroke-width] [:font-size])` | 定义命名字样式 |
| `use-style` | `(use-style style-name . shapes)` | 将样式应用到图形对象 |

### 示例

```scheme
(define-style 'button-primary
  :fill "#3498db" :stroke "#2980b9" :stroke-width 2)
(define-style 'button-danger
  :fill "#e74c3c" :stroke "#c0392b" :stroke-width 2)

; 应用样式
(define btn (use-style 'button-primary
              (rect 0 0 120 40 :rx 6)))
```

### 预定义样式

| 样式名 | 效果 |
|--------|------|
| `cel` | 纯色填充，无描边——卡通风格 |
| `pixel` | 纯色填充，1px 描边——像素风格 |
| `flat` | 纯色填充，2px 描边——扁平 UI |

---

## Palette 调色板

| 函数 | 签名 | 说明 |
|------|------|------|
| `define-palette` | `(define-palette name '((key color) ...))` | 定义命名调色板 |
| `palette-ref` | `(palette-ref name key)` | 从调色板取颜色 |

### 示例

```scheme
(define-palette 'forest
  '((bg     "#1a1a2e")
    (ground "#16213e")
    (tree   "#0f3460")
    (leaf   "#e94560")))

(define tree
  (group
    (rect 20 30 8 20 :fill (palette-ref 'forest 'tree))
    (circle 24 20 16 :fill (palette-ref 'forest 'leaf))))
```

---

## Sprite Components

预定义的语义化游戏组件，快速组装角色和 UI。

### 角色组件

| 函数 | 签名 | 说明 |
|------|------|------|
| `character` | `(character [:head] [:body] [:eyes] [:hair] [:mouth] [:outfit])` | 完整角色 |
| `body` | `(body width height [:fill])` | 身体 |
| `head` | `(head radius [:fill])` | 头部 |
| `eyes` | `(eyes [:fill] [:style])` | 眼睛 |
| `hair` | `(hair style [:fill] [:length])` | 头发 |
| `mouth` | `(mouth style [:fill])` | 嘴巴 |
| `outfit` | `(outfit style [:fill])` | 服装 |

### 装备与物品

| 函数 | 签名 | 说明 |
|------|------|------|
| `weapon` | `(weapon type [:fill] [:length])` | 武器 |
| `potion` | `(potion [:fill] [:size])` | 药水 |
| `chest` | `(chest [:fill] [:size])` | 宝箱 |
| `generic-item` | `(generic-item shape [:fill] [:size])` | 通用物品 |

### UI 组件

| 函数 | 签名 | 说明 |
|------|------|------|
| `button` | `(button label [:width] [:height])` | 按钮 |
| `panel` | `(panel [:width] [:height] [:fill])` | 面板 |
| `health-bar` | `(health-bar value max [:width] [:height])` | 血条 |
| `icon` | `(icon type [:size] [:fill])` | 图标 |

### 场景组件

| 函数 | 签名 | 说明 |
|------|------|------|
| `tile` | `(tile shape [:fill])` | 瓦片（用于 tilemap） |
| `decoration` | `(decoration type [:fill] [:size])` | 装饰物 |
| `background` | `(background type [:fill])` | 背景 |

### 完整角色示例

```scheme
(character
  :body (body 30 40 :fill "#3498db")
  :head (head 15 :fill "#f39c12")
  :eyes (eyes :fill "#2c3e50" :style 'happy)
  :hair (hair 'spiky :fill "#e74c3c" :length 8)
  :outfit (outfit 'knight :fill "#95a5a6"))
```

注意：部分组件仍在开发中，传入未知样式或类型会 fallback 到默认形状。
