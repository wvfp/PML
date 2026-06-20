# PML 原生 3D 支持设计文档

**日期**: 2026-06-20  
**状态**: 待实现  
**范围**: C++ PML 运行时（`src/pml/`）

---

## 1. 目标

在 PML 中引入原生 3D 原语支持，使用户可以：

- 在 3D 空间定义基本形状（立方体、长方体、倒角立方体/长方体、锥形、平面、球）。
- 为每个面绑定任意 PML 2D 图形作为贴图。
- 对 3D 对象进行旋转、平移、缩放。
- 配置可移动相机，支持透视（perspective）与正交（orthographic）两种投影，且可切换。

本设计不导入外部 mesh，也不追求通用可编程 3D 原语；优先解决 2.5D isometric tilemap 等场景中的真实 3D 旋转需求。

**坐标系约定**：右手坐标系，`+Y` 向上，`+Z` 指向屏幕外（朝向观察者）。相机默认位于 `+Z` 轴，朝向原点。

---

## 2. 非目标

- 外部模型导入（`.obj`、`.gltf` 等）。
- 通用可编程 mesh（用户手写顶点/面）。
- 复杂光照、阴影、PBR 材质。
- 物理、碰撞检测。
- GPU 硬件加速的完整 3D 管线（仍基于 Skia 2D 后端做软件投影+贴图）。

---

## 3. PML 用户层 API

### 3.1 全局相机

```pml
(camera :position '(150 150 300)
        :target   '(150 150 0)
        :up       '(0 1 0)
        :fov      45
        :near     1
        :far      1000
        :projection 'perspective)   ; 或 'orthographic
```

正交模式额外参数：

```pml
(camera :position '(0 0 300)
        :target   '(0 0 0)
        :projection 'orthographic
        :size     200)              ; 视口高度（ world units ）
```

相机状态为全局单例，渲染时所有 `mesh3d` 对象共用该相机。若用户未设置相机，使用默认正交相机。

### 3.2 3D 原语

#### 立方体 / 长方体

```pml
(cube3d :size 80
        :front  face-graphic
        :back   face-graphic
        :left   face-graphic
        :right  face-graphic
        :top    face-graphic
        :bottom face-graphic)

(cuboid3d :width 80 :height 60 :depth 100
          :front face-graphic
          ...)
```

#### 倒角立方体 / 倒角长方体

```pml
(rounded-cuboid3d :width 80 :height 80 :depth 80
                  :radius 8
                  :segments 4
                  :front face-graphic
                  ...)
```

#### 锥形

```pml
(cone3d :radius 40 :height 80 :segments 32
        :side face-graphic
        :base face-graphic)
```

#### 平面

```pml
(plane3d :width 80 :depth 80 :material face-graphic)
```

#### 球

```pml
(sphere3d :radius 40 :segments 32 :rings 16
          :material face-graphic)
```

球体使用经纬度细分，整个球只支持单一 `material`；若需要分面贴图，用户可用 `cuboid3d` 近似。

### 3.3 3D 变换

```pml
(rotate-x obj angle)
(rotate-y obj angle)
(rotate-z obj angle)
(translate3d obj x y z)
(scale3d obj sx sy sz)
```

变换函数返回一个新的 3D `GraphicObject`，不修改原对象（保持不可变性）。变换在模型空间叠加，顺序为：缩放 → 旋转 → 平移（与 2D `AffineTransform` 的 compose 语义一致）。

### 3.4 渲染

3D 对象通过现有 `add` 加入 canvas，通过现有 `render` 输出：

```pml
(canvas 400 400 :bg "#E8E0D5")
(add (rotate-y tile 30))
(render "tile.png")
```

---

## 4. C++ 架构

### 4.1 新增模块

在 `src/pml/` 下新建目录 `graphics3d/`：

| 文件 | 职责 |
| --- | --- |
| `vec3.h` / `.cpp` | `Vec3` 3D 向量/点，含基本运算。 |
| `mat4.h` / `.cpp` | `Mat4` 4×4 矩阵，封装 translate/rotate/scale/perspective/orthographic/lookAt。 |
| `transform3d.h` / `.cpp` | `Transform3D` 组合模型变换（TRS）。 |
| `camera3d.h` / `.cpp` | `Camera3D` 相机 + 投影矩阵。 |
| `mesh3d.h` / `.cpp` | `Mesh3D`：顶点数组、面数组、每面 material（`GraphicObject`）。 |
| `primitive_factory.h` / `.cpp` | 生成 cube、cuboid、rounded-cuboid、cone、plane、sphere 的 `Mesh3D`。 |
| `builtins_3d.h` / `.cpp` | PML 内置函数实现：`camera`、`cube3d`、`cuboid3d`、`rotate-x/y/z` 等。 |
| `CMakeLists.txt` | 模块构建配置。 |

### 4.2 现有类型扩展

#### `GraphicObject`（`src/pml/graphics/objects.h`）

新增 `shape_type == "mesh3d"`，`params` 中存储：

- `"mesh"` → `std::shared_ptr<Mesh3D>`
- `"transform"` → `std::shared_ptr<Transform3D>`（默认为单位变换）

保持 `GraphicObject` 的不可变语义：`rotate-y` 等函数返回一个新的 `GraphicObject`，仅替换 `transform`。

#### `Box::Kind`（`src/pml/core/types.h`）

不新增 Box kind。`Mesh3D`、`Transform3D`、`Camera3D` 通过 `std::shared_ptr` 直接挂在 `GraphicObject::params`（`Value`）中。`Value` 已能容纳 `GraphicObject`、`Canvas`、`AffineTransform` 等，params 使用 `Value` 可自然承载共享指针。

若后续 3D 类型需要作为独立一等公民返回，再考虑扩展 `Value`。

### 4.3 注册

在 `src/pml/api/api.cpp` 的 `PMLRuntime::init_global_env()` 中调用：

```cpp
register_3d_builtins(m_env);
```

---

## 5. 渲染管线（Skia 后端）

### 5.1 总体流程

1. **收集 mesh**：遍历 canvas 中所有 `shape_type == "mesh3d"` 的 `GraphicObject`。
2. **模型变换**：将 `Mesh3D` 顶点从局部空间变换到世界空间（`Transform3D`）。
3. **视图/投影**：用 `Camera3D` 的 `lookAt` + `projection` 矩阵将世界坐标投影到 NDC，再映射到屏幕坐标。
4. **背面剔除**：对每个三角面，计算投影后 2D 叉积，顺时针（或逆时针，依坐标系约定）的面视为背面并丢弃。
5. **深度排序**：按面中心在世界空间/相机空间的 Z 值从远到近排序（画家算法）。
6. **光栅化 material**：对每个可见面的 material `GraphicObject`，渲染到离屏 `SkSurface` 得到 `SkBitmap`。
7. **绘制面**：使用 `SkVertices` 将 bitmap 按 UV 贴到投影后的多边形（四边形拆为两个三角形）。

### 5.2 贴图映射

每个面在 `Mesh3D` 中保存：

- 顶点索引（3 或 4 个）
- 每个顶点的 UV 坐标
- material `GraphicObject`

材质坐标系：material 2D 图形以自身局部坐标系绘制到 `SkSurface`；UV `[0,1]` 覆盖整张 surface。

- 对 `cube3d`/`cuboid3d`，每个面默认使用 `size × size`（或面宽 × 面高）的 surface，material 在 `[0, width] × [0, height]` 局部坐标中定义。
- 对 `cone3d`、`sphere3d` 等曲面，surface 大小由形状参数决定（如圆锥侧面包裹为 `2πr × h` 的矩形区域），material 在 `[0, w] × [0, h]` 中定义。
- 用户可通过 `:uv-scale`、`:uv-offset` 等可选参数调整映射（本期可先不暴露，留扩展接口）。

### 5.3 深度与遮挡

- 使用**画家算法**（由远及近绘制）。
- 对凸形体（立方体、球等）足够；若未来支持凹形或透明面，再考虑 Z-buffer。
- 背面剔除默认开启，可通过 `camera` 参数 `:backface-culling #t/#f` 控制（默认 `#t`）。

### 5.4 正交模式用于 isometric

正交相机下，用户可自由旋转对象而不会出现透视畸变，直接生成 2.5D tilemap 素材。默认正交相机设置便于在任何 PML 文件中组合 `cube3d`、`rotate-y`、`camera` 等 API 实现 2.5D 效果。

---

## 6. 与现有 2D 系统的边界

- 现有 2D 图形（`circle`、`rect`、`polygon` 等）完全不受影响。
- `with-transform` 仍只处理 2D `AffineTransform`；3D 变换使用新的 `rotate-x/y/z` 等函数。
- `Canvas` 中 2D 对象与 3D 对象按 `add` 顺序绘制；3D 对象在渲染阶段统一投影绘制，2D 对象按原流程绘制。混合使用时建议先 `add` 背景 2D 元素，再 `add` 3D 对象。
- `render` 输出 PNG/GIF 的接口不变。

---

## 7. 实现顺序

按优先级分阶段实现，每阶段可独立验证：

1. **数学基础**：`Vec3`、`Mat4`、`Transform3D`、`Camera3D`（含透视/正交矩阵）。
2. **Mesh3D + 立方体**：实现 `cube3d`、`cuboid3d`，单面纯色/简单 material 渲染。
3. **贴图支持**：将 PML 2D `GraphicObject` 渲染为 `SkBitmap`，用 `SkVertices` 贴到 3D 面。
4. **变换内置函数**：`rotate-x/y/z`、`translate3d`、`scale3d`。
5. **更多形状**：`rounded-cuboid3d`、`cone3d`、`plane3d`、`sphere3d`。
6. **全局相机 API**：`camera` 函数，正交/透视切换。
7. **示例**：提供独立的 3D 旋转示例 PML 文件（不绑定到具体项目目录）。
8. **测试**：`tests/builtins_smoke.cpp` 增加 3D 渲染断言（至少验证无 crash、输出非空）。

---

## 8. 示例：3D 立方体旋转

```pml
; cube3d_demo.pml
(define face-size 80)

(define (make-grass-face)
  (group
    (rect 0 0 face-size face-size :fill "#7CB342" :stroke "#33691E")
    (circle 40 40 5 :fill "#9CCC65" :stroke "none")))

(define base-cube
  (cube3d :size face-size
          :front (make-grass-face)
          :back  (make-grass-face)
          :left  (make-grass-face)
          :right (make-grass-face)
          :top   (make-grass-face)
          :bottom (make-grass-face)))

(define (draw-cube-yaw cx cy yaw)
  (translate3d (rotate-y base-cube yaw) cx cy 0))

(set-backend! "skia")

(camera :position '(0 0 300)
        :target   '(0 0 0)
        :projection 'orthographic
        :size 200)

(canvas 300 300 :bg "#E8E0D5")

(define frame-idx 0)
(define total-frames 24)

(every-frame
  (lambda ()
    (clear-canvas)
    (define t (/ frame-idx total-frames))
    (define yaw (* 40 (sin (* t 2 3.1415926535))))
    (add (draw-cube-yaw 150 150 yaw))
    (set! frame-idx (+ frame-idx 1))))

(render "rotate.gif" :fps 16 :duration 1.5)
```

---

## 9. 风险与决策

| 风险 | 缓解 |
| --- | --- |
| Skia 2D 后端不支持原生 3D，贴图映射需手写。 | 使用 `SkVertices` + `SkBitmap`，四边形拆三角面。 |
| 球体细分多面数可能性能差。 | 默认适中细分（32×16），用户可调整 `:segments`/`:rings`。 |
| material 图形坐标系与 UV 映射易出错。 | 统一规定 material 在 `[0, size]` 局部坐标中定义；立方体每面独立 material。 |
| 凹形体遮挡错误。 | 本期不支持凹形；仅提供凸形体。 |
| CJK 文字贴图仍受 `text` 限制。 | 保持现状，文字作为 material 时仍只支持 ASCII/Latin。 |

---

## 10. 待实现 checklist

- [ ] 创建 `src/pml/graphics3d/` 目录与 `CMakeLists.txt`
- [ ] 实现 `Vec3`、`Mat4`、`Transform3D`、`Camera3D`
- [ ] 实现 `Mesh3D` 数据结构
- [ ] 实现 `cube3d`、`cuboid3d` 原语工厂
- [ ] 扩展 `GraphicObject` 支持 `mesh3d` shape type
- [ ] Skia 后端：投影、背面剔除、深度排序、`SkVertices` 贴图绘制
- [ ] 实现 `rotate-x/y/z`、`translate3d`、`scale3d`、`camera` 内置函数
- [ ] 实现 `rounded-cuboid3d`、`cone3d`、`plane3d`、`sphere3d`
- [ ] 在 `PMLRuntime::init_global_env()` 注册 3D 内置函数
- [ ] 更新 `tests/builtins_smoke.cpp`
- [ ] 提供独立 3D 旋转示例 PML 文件
- [ ] 更新 `AGENTS.md` 的“Implemented identically to Python”/差异列表
