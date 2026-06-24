# 11 — Render Channels (多通道素材导出)

## 概述

将 GraphicObject 渲染为多个通道文件（albedo/normal/specular），
适用于游戏素材管线的预烘焙环节。

---

## render-channels

```scheme
(render-channels graphic
  :output "prefix"
  :channels '(albedo specular normal)
  [:width W]
  [:height H]
  [:bg "transparent"])
```

| 参数 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `graphic` | GraphicObject | — | 要渲染的图形（必填，第一个位置参数） |
| `:output` | string | — | 输出文件前缀（必填 kwarg） |
| `:channels` | list | `'(albedo specular normal)` | 要输出的通道列表 |
| `:width` | int | 图形自然尺寸 | 输出图像宽度 |
| `:height` | int | 图形自然尺寸 | 输出图像高度 |
| `:bg` | string | `"transparent"` | 背景色 |

**返回**: 已创建文件路径的列表

---

## 通道说明

### Albedo (漫反射)

原样渲染 GraphicObject，无修改。

- 输出文件: `{prefix}_albedo.png`
- 包含原始颜色和透明度

### Specular (高光)

用颜色的亮度值替换所有 fill/stroke。

```
L = 0.299R + 0.587G + 0.114B
```

- 输出文件: `{prefix}_specular.png`
- 灰度图像，Alpha 保留
- 高亮度区域 = 强高光反射

### Normal (法线)

默认将所有形状的法线设为 `(0, 0, 1)`（Z 轴正方向）。

```
法线编码: RGB = (nx*127+128, ny*127+128, nz*127+128)
默认: nx=0, ny=0, nz=1 → RGB(128, 128, 255)
```

- 输出文件: `{prefix}_normal.png`
- 蓝紫色调（默认朝前法线）
- 保留 Alpha
- 未来可扩展按形状设定自定义法线方向

---

## 完整示例

```scheme
; 定义一个剑
(define sword (group
  (rect 30 5 8 50 :fill "#bdc3c7")          ; 剑刃
  (polygon 34 55 25 75 43 75 :fill "#95a5a6"); 剑尖
  (rect 28 75 12 15 :fill "#8B4513")        ; 剑柄
  (rect 25 75 18 5 :fill "#DAA520")))        ; 剑格

; 三通道导出
(render-channels sword
  :output "sword"
  :channels '(albedo specular normal))

; → sword_albedo.png   (原始颜色)
; → sword_specular.png (灰度亮度)
; → sword_normal.png   (法线贴图)

; 自定义尺寸
(render-channels (circle 50 50 40 :fill "red")
  :output "circle"
  :channels '(albedo normal)
  :width 128 :height 128)
```

---

## 误差处理

| 错误 | 原因 |
|------|------|
| `"not implemented channel"` | 通道名不在 `'(albedo specular normal)` 中 |
| `"failed to create surface"` | 后端无法创建渲染表面 |
| `"first argument must be a GraphicObject"` | 第一个参数不是 GraphicObject |
