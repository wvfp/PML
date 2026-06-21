# 14 — 3D Graphics

## 概述

PML C++ 端口支持原生 3D 基本体渲染，Python 版无此功能。
3D 图形作为特殊 GraphicObject（`mesh3d` shape_type）存在，
与 2D 图形在同一渲染管线中处理。

---

## 3D 基本体

所有 3D 基本体返回 GraphicObject。

| 函数 | 签名 | 说明 |
|------|------|------|
| `cube3d` | `(cube3d size [:fill] [:stroke])` | 立方体 |
| `cuboid3d` | `(cuboid3d w h d [:fill] [:stroke])` | 长方体 |
| `rounded-cuboid3d` | `(rounded-cuboid3d w h d r [:fill] [:stroke])` | 圆角长方体 |
| `cone3d` | `(cone3d radius height [:fill] [:stroke])` | 圆锥体 |
| `plane3d` | `(plane3d w h [:fill] [:stroke])` | 平面 |
| `sphere3d` | `(sphere3d radius [:fill] [:stroke])` | 球体（经纬线细分） |

### 材质映射

每个 3D 基本体支持每个面独立设置 2D PML 材质（在侧边渲染的图案）：

```scheme
; 为每个面指定材质
(cuboid3d 2 2 2
  :fill "#e74c3c"
  :material-face-pos-x (rect 0 0 1 1 :fill "blue")   ; 右
  :material-face-neg-x (rect 0 0 1 1 :fill "green")  ; 左
  :material-face-pos-y (rect 0 0 1 1 :fill "red")    ; 上
  :material-face-neg-y (rect 0 0 1 1 :fill "yellow") ; 下
  :material-face-pos-z (rect 0 0 1 1 :fill "cyan")   ; 前
  :material-face-neg-z (rect 0 0 1 1 :fill "purple")) ; 后
```

---

## 3D 变换

| 函数 | 签名 | 说明 |
|------|------|------|
| `rotate-x` | `(rotate-x angle)` | 绕 X 轴旋转（弧度） |
| `rotate-y` | `(rotate-y angle)` | 绕 Y 轴旋转 |
| `rotate-z` | `(rotate-z angle)` | 绕 Z 轴旋转 |
| `translate3d` | `(translate3d x y z)` | 3D 平移 |
| `scale3d` | `(scale3d sx sy sz)` | 3D 缩放 |

```scheme
; 旋转立方体
(define box (cube3d 1 :fill "#e74c3c"))
(define rotated-box (with-transform box
                       (compose (rotate-x 0.5) (rotate-y 0.3))))
```

---

## 相机

```scheme
(camera :position '(x y z)
        :target '(x y z)
        [:up '(ux uy uz)]
        [:fov 60]
        [:projection 'perspective|'orthographic]
        [:near 0.1]
        [:far 100.0]
        [:size 5.0]          ; 正交投影的视野大小
        [:backface-culling true])
```

| 参数 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `:position` | list | `(0 0 5)` | 相机位置 |
| `:target` | list | `(0 0 0)` | 观察目标点 |
| `:up` | list | `(0 1 0)` | 上方向 |
| `:fov` | number | 60 | 透视投影视角（度） |
| `:projection` | symbol | `'perspective` | 投影类型 |
| `:size` | number | 5 | 正交投影的可见区域高度 |
| `:near`, `:far` | number | 0.1, 100 | 近远裁剪面 |
| `:backface-culling` | bool | `true` | 背面剔除 |

### 示例

```scheme
; 设置透视相机
(camera :position '(5 4 6)
        :target '(0 0 0)
        :projection 'perspective
        :fov 45)

; 构建并渲染 3D 场景
(define scene (group
  (cube3d 1 :fill "#e74c3c" :stroke "#c0392b")
  (sphere3d 0.7 :fill "#3498db" :stroke "#2980b9"
    :material-face-pos-x (circle 0 0 0.3 :fill "white"))))

; 用 Canvas 过渡到 2D
(let ((c (canvas)))
  (add c scene)
  (render c "3d_scene.png"))
```

---

## 注意事项

- 3D 图形渲染走独立的 `draw_mesh_skia()` 路径，与 2D 形状分开
- 材质（Material Faces）使用 2D GraphicObject 映射到 3D 表面 UV 空间
- 相机为全局单例，设置后对后续所有 3D 图形生效
- 3D 图形可以混入 2D group 中，渲染时会自动调度到正确路径
- 不支持光源——所有材质颜色为自发光（unlit）
- 不支持骨骼动画、蒙皮或碰撞检测
