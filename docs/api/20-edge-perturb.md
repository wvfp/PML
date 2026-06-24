# 20 — Edge Perturbation & Corner Rounding (多边形顶点扰动与圆角)

## 概述

为 PML 的 `polygon` 形状添加边缘顶点扰动和圆角功能，让多边形轮廓产生自然的不规则感。

**适用场景**：等距瓦片边缘自然化、手绘风格多边形、有机形状生成。

**管线**: `perturb-polygon`（获取扰动顶点列表）→ `polygon`（绘制）或自行构建侧面。

---

## perturb-polygon — 多边形扰动

```scheme
(perturb-polygon vertices
  [:edge-noise N]
  [:edge-mask '(#t #t ...)]
  [:edge-subdiv N]
  [:corner-radius N]
  [:corner-mask '(#t #t ...)]
  [:seed N])
```

### 参数

| 参数 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `vertices` | list | — | 顶点列表 `((x y) (x y) ...)`（必填，至少 3 个顶点） |
| `:edge-noise` | double \| list | `0.0` | 每条边的噪声幅度。标量=所有边统一，向量=按边独立。幅度 = `edge-noise × 边长` |
| `:edge-mask` | bool \| list | `#t` | 是否对边施加噪声。`#f` 的边保持直线 |
| `:edge-subdiv` | int \| list | `0` | 每条边的 Catmull-Rom 细分点数。值=N 表示在边中间插入 N 个细分点 |
| `:corner-radius` | double \| list | `0.0` | 圆角半径（相对值，实际半径 = value × min(相邻边边长)），自动 clamp 到相邻边较短者的一半 |
| `:corner-mask` | bool \| list | `#t` | 是否对顶点做圆角 |
| `:seed` | int | `0` | 随机种子，确定性噪声 |

### 返回值

嵌套列表 `((x y) (x y) ...)`，可直接传入 `polygon`。

每条边独立扰动后拼接（相邻边共享的顶点会重复），`corner-radius` > 0 时尖角被贝塞尔弧线替代。

### 示例

```scheme
; 基本用法：4 顶点正方形加噪声
(define pts '((0 0) (100 0) (100 100) (0 100)))
(define perturbed (perturb-polygon pts :edge-noise 0.15 :edge-subdiv 3 :seed 42))
(add (polygon perturbed :fill "#5DAE3B" :stroke "#333" :stroke-width 2))

; 按边独立控制：只扰动边 1 和 3
(perturb-polygon pts
  :edge-noise '(0.0 0.2 0.0 0.15)
  :edge-mask '(#f #t #f #t)
  :edge-subdiv '(0 3 0 2))

; 带圆角
(perturb-polygon pts
  :edge-noise 0.1
  :edge-subdiv 2
  :corner-radius 8.0
  :corner-mask '(#t #t #t #t)
  :seed 42)
```

---

## polygon 直接 kwarg

`polygon` 形状支持相同的扰动参数作为可选 kwargs，在图形创建时直接应用扰动。

```scheme
(polygon pts
  [:edge-noise N] [:edge-mask M]
  [:edge-subdiv N] [:corner-radius R]
  [:corner-mask M] [:edge-seed N]
  ...其他样式参数)
```

| 参数 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `:edge-noise` | double \| list | — | 不传=不扰动；传了=启用扰动 |
| `:edge-mask` | bool \| list | `#t` | 噪声掩码 |
| `:edge-subdiv` | int \| list | `0` | 细分点数 |
| `:corner-radius` | double \| list | `0.0` | 圆角半径 |
| `:corner-mask` | bool \| list | `#t` | 圆角掩码 |
| `:edge-seed` | int | hash(顶点列表) | 随机种子，不传时自动从顶点列表生成确定性种子 |

**注意**：若未传入任何 `edge-*` 或 `corner-*` 参数，`polygon` 行为完全不变（零回归）。

### 示例

```scheme
; 带扰动的基本多边形
(add (polygon '((0 0) (100 0) (100 100) (0 100))
              :edge-noise 0.15
              :edge-subdiv 3
              :edge-seed 42
              :fill "#e74c3c"
              :stroke "#333"
              :stroke-width 2))

; 圆角多边形
(add (polygon '((0 0) (100 0) (100 100) (0 100))
              :corner-radius 8.0
              :edge-subdiv 1
              :fill "#3498db"))
```

---

## 边索引规则

顶点列表 `(v0 v1 v2 ... vn-1)` 构成 n 边形：

```
         v1 ─────── v2
          ╱            ╲
      边 1              边 2
        ╱                ╲
      v0 ───────────────── v3
         边 0 / 边 n-1
```

- **边 i** = 从 `v[i]` 到 `v[(i+1) % n]` 的线段
- **角 i** = 边 `i-1` 与边 `i` 的交点（顶点 `v[i]`）
- 参数向量长度应 == 顶点数（若使用向量传参）

对于正方形 `((0 0) (100 0) (100 100) (0 100))`：
| 索引 | 边 | 角（顶点） |
|------|----|-----------|
| 0 | (0,0)→(100,0) | 顶部边缘 | v0=(0,0) |
| 1 | (100,0)→(100,100) | 右侧边缘 | v1=(100,0) |
| 2 | (100,100)→(0,100) | 底部边缘 | v2=(100,100) |
| 3 | (0,100)→(0,0) | 左侧边缘 | v3=(0,100) |

---

## 参数类型：标量 vs 向量

所有扰动参数支持两种传参方式：

| 传参方式 | 语法 | 效果 |
|----------|------|------|
| **标量** | `:edge-noise 0.15` | 所有边使用同一值 |
| **向量** | `:edge-noise '(0.1 0.0 0.2 0.0)` | 每条边独立取值 |

向量长度应等于多边形边数（= 顶点数）。若短于边数，缺失值用 `0` 或 `#f` 填充。

**典型应用**：等距瓦片的"拼接边"保持笔直（`:edge-mask '(#f #t #f #t)`），仅扰动暴露边。

---

## 组合示例

### 等距瓦片（菱形）+ 不规则轮廓

```scheme
(define (isometric-tile col row)
  ;; 菱形顶点
  (let ((cx (* col 64))
        (cy (* row 32)))
    (list (list cx (+ cy 32))          ; 底部
          (list (+ cx 32) cy)          ; 右
          (list cx (- cy 32))          ; 顶部
          (list (- cx 32) cy))))       ; 左

;; 扰动暴露边（索引 0 和 2），保持拼接边（1 和 3）笔直
(define pts (isometric-tile 5 3))
(define perturbed
  (perturb-polygon pts
    :edge-noise '(0.1 0.0 0.1 0.0)
    :edge-subdiv '(3 0 3 0)
    :edge-mask '(#t #f #t #f)
    :seed (* col 1000 row)))

(add (polygon perturbed :fill "#5DAE3B" :stroke "#333" :stroke-width 1))
```

### perturb-polygon + polygon 叠加

```scheme
;; 先用 perturb-polygon 获取扰动顶点，再传给 polygon 加额外的扰动层
(define base-pts '((0 0) (100 0) (100 100) (0 100)))
(define noisy-pts (perturb-polygon base-pts
                    :edge-noise 0.1
                    :edge-subdiv 2
                    :seed 1))
(add (polygon noisy-pts
              :edge-noise 0.05               ; 第二层扰动
              :edge-seed 2                    ; 不同种子
              :fill "#8e44ad"
              :stroke "#333"))
```

### 自定义侧面（extrude-tile 替代方案）

```scheme
;; 用 perturb-polygon 获得不规则顶面，手动构建侧面
(define top-pts (perturb-polygon '((0 0) (80 0) (80 40) (0 40))
                  :edge-noise 0.15
                  :edge-subdiv 3
                  :seed 42))

;; 顶面
(add (polygon top-pts :fill "#5DAE3B"))

;; 侧面（用顶面的底部边 + 偏移底部边）
(define bottom-pts (map (lambda (pt) (list (car pt) (+ (cadr pt) 30))) top-pts))
;; ... 用 PML 循环构建侧面的四边形
```

---

## 示例画廊

`examples/12-edge-perturb/` 目录包含 5 个示例脚本，覆盖从入门到高级的全部特性：

| # | 文件 | 说明 |
|---|------|------|
| 01 | `01_basic_perturb.pml` | 基础用法：三角形、正方形、五边形对比 exact vs perturbed，含 `perturb-polygon` + `polygon` kwarg 两种 API |
| 02 | `02_edge_mask_and_vector.pml` | 掩码与向量控制：`edge-mask` 选择性扰动边、`edge-noise`/`edge-subdiv` 按边独立传参 |
| 03 | `03_corner_rounding.pml` | 圆角效果：不同 `corner-radius` 对比、`corner-mask` 选择性圆角、圆角+噪声叠加 |
| 04 | `04_isometric_tiles.pml` | 等距瓦片自然化：仅扰动上下暴露边、保留左右拼接边笔直，与 exact 瓦片对比 |
| 05 | `05_showcase.pml` | 综合展示：种子变化、有机形状、双层叠加扰动、混合边模式 |

渲染命令（在项目根目录执行）：

```bash
# 进入目录后再渲染（pml.exe 将输出写入当前目录）
cd examples/12-edge-perturb
& "G:\Project\PML\build\debug\src\pml\cli\Debug\pml.exe" 01_basic_perturb.pml

# 或在项目根目录使用绝对路径，将 CWD 设为输出目录：
cd G:\Project\PML\examples\12-edge-perturb
Get-ChildItem *.pml | ForEach-Object {
    & "G:\Project\PML\build\debug\src\pml\cli\Debug\pml.exe" $_.FullName
}
```

### 等距瓦片效果预览

`04_isometric_tiles.pml` 展示了边扰动最典型的应用场景：在 4×3 等距瓦片网格中，仅对每块瓦片的上下边（暴露边）添加扰动，左右边（拼接边）保持笔直，从而：
- ✅ 相邻瓦片无缝拼接
- ✅ 整体轮廓获得自然的不规则感
- ✅ 每个瓦片可独立设置随机种子

---

## 实现细节

- 扰动算法使用 **Catmull-Rom 样条**插值细分边（需至少 3 个控制点）
- **圆角**使用三次贝塞尔近似圆弧，步数 = ceil(θ × 4 / π) + 1（直角约 5 个点）
- **噪声**为确定性 CPU 2D Perlin 噪声，基于世界坐标计算——相同坐标得到相同位移，保证相邻瓦片共享边连续
- 扰动在图形构建时完成，渲染后端看到的是已扰动顶点——无需后端修改
- 参数存储在 GraphicObject 的 `metadata` 中，不修改 `ParamKey` 枚举
