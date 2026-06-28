# PML 地形过渡渲染 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 实现 PML 地形过渡渲染所需的 9 项增强功能（排除 F3「可编程着色器」——已通过 `(shader "SkSL...")` 支持），使 tilemap 系统能实现边缘对齐的噪声过渡、渐变填充、裁剪、透视映射等能力。

**Architecture:** 改动涉及 evaluator（新 builtin 函数、特殊形式扩展）、graphics（GraphicObject 新字段/结构）、backend/skia（Skia 渲染管线）。`tests/` 中新增相应的 GTests。

**Tech Stack:** C++23, Skia (pre-built), Googletest

## Global Constraints

- 所有修改遵循 CLAUDE.md 中定义的代码风格（4-space indent, 100 cols, LLVM-based .clang-format）
- `Result<T>` 错误处理模式，使用 `type_error()`/`syntax_error()`/`arity_error()` 工厂函数
- `Arena` bump allocator 用于 execute()-lifetime 数据；`make_list_heap()` 用于持久对象
- 后端修改在 `skia_backend_draw.cpp` 和 `skia_backend_internal.h` 中
- 测试在 `tests/` 目录中添加，通过 `gtest_discover_tests` 集成到 CMake

## Feature Inventory

需求文档中的 10 项功能，实际状态：

| # | 功能 | 状态 | 计划动作 |
|---|------|------|---------|
| F1 | 世界坐标 Shader | 缺失 | `apply-shader!` 扩展 `:coordinate-space :world` kwarg |
| F2 | Shader→Texture | 缺失 | 新特殊形式 `render-to-texture` 或扩展 `define-texture` |
| F3 | 可编程着色器 | **已有** | `(shader "SkSL...")` 支持任意 SkSL——跳过 |
| F4 | 渐变填充 | 缺失 | 新 Gradient 类型 + `linear`/`radial` 函数 + Skia 渲染 |
| F5 | 裁剪/蒙版 | 缺失 | 新 `with-clip` 特殊形式 + Skia clipPath 渲染 |
| F6 | 混合模式在 add/group | 缺失 | `add`/`group` 关键字参数 `:blend-mode` |
| F7 | mod/rem/quotient | **已有(部分)** | `modulo`/`remainder`/`quotient` 已存在；只需加 `mod`/`rem` 别名 |
| F8 | 位运算 | 缺失 | 添加 `bit-and`/`bit-or`/`bit-xor`/`bit-not`/`bit-shift` |
| F9 | 按名称检索对象 | 缺失 | 名称注册表 + `find`/`set-property` 函数 |
| F10 | 透视纹理映射 | 缺失 | `texture-map` 扩展 `:perspective-correction` 参数 |

## 文件结构

### 新增文件
| 文件 | 归属 |
|------|------|
| `src/pml/evaluator/gradient_builtins.cpp` | F4 — `linear`/`radial` gradient builtins |
| `src/pml/graphics/gradient.h` | F4 — Gradient 数据结构 |
| `src/pml/evaluator/clip_builtins.cpp` | F5 — `with-clip` 特殊形式 |
| `src/pml/graphics/name_registry.h` | F9 — 名称注册表 |

### 修改文件
| 文件 | 涉及功能 |
|------|---------|
| `src/pml/evaluator/builtins_arithmetic.cpp` | F7, F8 |
| `src/pml/evaluator/shader_builtins.cpp` | F1 |
| `src/pml/evaluator/texture_builtins.cpp` | F2 |
| `src/pml/evaluator/builtins_graphics.cpp` | F4 (gradient builtins 注册), F6 |
| `src/pml/graphics/objects.h` | F4 (gradient 字段), F6 (blend_mode 传递) |
| `src/pml/backend/skia/skia_backend_draw.cpp` | F1, F4, F5, F10 |
| `src/pml/backend/skia/skia_backend_internal.h` | F1, F4, F5 |
| `src/pml/api/api.cpp` | F9 (PMLContext 初始化) |
| `src/pml/api/pml_context.h` | F9 (name_registry 成员) |
| `src/pml/graphics/texture_map.h/cpp` | F10 |

### 测试文件
| 文件 | 涉及功能 |
|------|---------|
| `tests/arithmetic_test.cpp` | F7, F8 |
| `tests/shader_test.cpp` | F1 |
| `tests/gradient_test.cpp` | F4 |
| `tests/clip_test.cpp` | F5 |
| `tests/blend_test.cpp` | F6 |

## 数据流

```
                              F4: gradient builtins
                                    ↓
F7/F8: math/bit ops    F1: apply-shader! :coordinate-space :world
    ↓                          ↓
evaluator → GraphicObject → SkiaBackend → PNG/GIF
                ↓                ↓
         F6: blend_mode    F5: clipPath
         F4: fill_gradient F10: SkMatrix::setPersp
                ↓
         F2: shader→offscreen→texture
                ↓
         F9: name registry (PMLContext)
```

---

## Phase 0: 基础数学与位运算（P0，约 1 小时）

> 这是最直接的功能，几乎没有风险，优先完成以扫清路径。

### F7: mod/rem 别名

**现状:** `modulo`, `remainder`, `quotient` 已在 `builtins_arithmetic.cpp:130-202` 实现。`mod` 和 `rem` 是文档中建议的名称但未注册。

**改动:**
- `src/pml/evaluator/builtins_arithmetic.cpp` — 在 `modulo` 注册后加一行 `def("mod", ...)` 指向同一 lambda；同样 `def("rem", ...)` → `remainder`

**复杂度:** 极低，各加一行别名定义。

### F8: 位运算

**现状:** 完全缺失。需求文档需要的 5 个函数。

**改动:**
在 `builtins_arithmetic.cpp` 中 `register_arithmetic(Env& env)` 末尾添加：

```cpp
def("bit-and", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
    if (args.size() != 2) return arity_error(..., 2, args.size());
    if (args[0].tag() != Value::Tag::Int || args[1].tag() != Value::Tag::Int)
        return type_error("bit-and expected integer arguments");
    return Value(args[0].as_int() & args[1].as_int());
});
```

类似地：`bit-or` (|), `bit-xor` (^), `bit-not` (~, 一元), `bit-shift` (<< 或 >>, 靠 n 的正负决定方向)。

**测试:**
```
(bit-and 7 3)   → 3
(bit-or 4 1)    → 5
(bit-xor 7 5)   → 2
(bit-not 0)     → -1
(bit-shift 1 3) → 8
(bit-shift 8 -1)→ 4
```

**边界条件:** 负数按补码合理？`bit-not 0` = `-1`（64位补码）。`bit-shift` 左溢为 UB（C++ 对有符号左溢是 UB），需在 shift >= 63 时 clamp。

---

## Phase 1: 世界坐标 Shader（P0，约 3 小时）

> 这是解决「多边形分割后噪声不对齐」问题的核心功能。

### F1: apply-shader! :coordinate-space :world kwarg

**现状:** `apply-shader!` 在 `shader_builtins.cpp:77-102` 实现，它编译 shader 为 `SkRuntimeEffect` 并作为参数存入 GraphicObject。坐标由 Skia 后端在渲染时决定，目前总是使用形状局部坐标（形状包围盒）。

**方案:**
在 `apply-shader!` 特殊形式处理中，接受可选 `:coordinate-space` 参数，值可以是 `:world` 或 `:local`（默认）。将信息存为 GraphicObject 的 metadata（或 params） `"shader_coord_space" = Value("world"/"local")`。

**Skia 后端改动** (`skia_backend_draw.cpp`):

当前渲染流程在 `draw_shape()` 附近。对于 `Group` 类型，每个 shape 都有 `configure_fill_paint()` 调用，其中有 `shader` 参数传入。Shader 在 Skia 中使用 `SkRuntimeEffect::makeShader()` 创建，需要传入 `SkMatrix`（local matrix）。

关键改动：在创建 shader 时，如果 `"shader_coord_space" == "world"`，则 local matrix 使用 canvas 的总变换矩阵的逆矩阵乘以当前 shape 的局部变换，使 shader 的采样坐标映射到画布世界坐标。

具体实现：
1. `shader_builtins.cpp`: 解析 `:coordinate-space` kwarg，存入 `GraphicObject::params` 或 `metadata`
2. `skia_backend_internal.h` `configure_fill_paint()`: 新增 `const SkMatrix& world_matrix` 参数
3. `skia_backend_draw.cpp`: 在 `draw_shape()` 或各形状绘制函数中，传递正确的矩阵

**测试:**
```pml
;; 两个相邻矩形，各自应用世界坐标噪声，边缘应对齐
(let ((left (rect 0 0 100 200 :fill "white"))
      (right (rect 100 0 100 200 :fill "white"))
      (n (noise-fractal :seed 42)))
  (group
    (apply-shader! left n :coordinate-space :world)
    (apply-shader! right n :coordinate-space :world)))
```

---

## Phase 2: 渐变填充（P1，约 3-4 小时）

> 使多边形可以填充渐变色，取代多层透明覆盖层。

### F4: Gradient 数据结构与渲染

**新增文件:**
- `src/pml/graphics/gradient.h` — `Gradient` 数据结构

**设计:**

```cpp
enum class GradientType { Linear, Radial };

struct GradientStop {
    double position;     // 0.0–1.0
    std::string color;   // "#rrggbb"
};

struct Gradient {
    GradientType type{GradientType::Linear};
    // Linear: (x1,y1)→(x2,y2) in shape-local coords
    // Radial: center (cx,cy), radius r
    double x1{}, y1{}, x2{}, y2{};
    double cx{}, cy{}, r{1.0};
    std::vector<GradientStop> stops;
};
```

**GraphicObject 修改** (`objects.h`):
- 新增 `std::optional<Gradient> fill_gradient` 字段
- `with_fill_gradient()` 不可变"修改器"

**特殊形式** (`gradient_builtins.cpp` or `builtins_graphics.cpp`):

```pml
;; Register in evaluator:
(linear ((0.0 "#267A16") (1.0 "#4A3520")))
(linear :x1 0 :y1 0 :x2 0 :y2 1 ((0.0 "#267A16") (1.0 "#4A3520")))
(radial :cx 0.5 :cy 0.5 :r 0.5 ((0.0 "#74c44f") (1.0 "#267A16")))
```

这些函数返回 `Value::Box(Kind::Gradient)`。

**形状构造修改** (`builtins_graphics.cpp`):
- `circle`, `rect`, `polygon` 等形状构造函数扩展 `:fill-gradient` 关键字参数
- 如果同时指定 `:fill` 和 `:fill-gradient`，`:fill-gradient` 优先

**Skia 后端** (`skia_backend_draw.cpp`):
在 `configure_fill_paint()` 中，如果 `fill_gradient` 有值：
- `Linear`: `SkGradientShader::MakeLinear(point0, point1, colors, positions, count)`
- `Radial`: `SkGradientShader::MakeRadial(center, radius, colors, positions, count)`

**测试:**
```pml
;; 基本线性渐变
(polygon '((0 0) (100 0) (100 100) (0 100))
  :fill-gradient (linear ((0.0 "red") (1.0 "blue"))))

;; 径向渐变
(circle 50 50 50
  :fill-gradient (radial :cx 50 :cy 50 :r 50
                  ((0.0 "yellow") (1.0 "orange"))))
```

---

## Phase 3: Shader→Texture 渲染（P1，约 2-3 小时）

> 捕获 shader 渲染结果为纹理，支持 UV 映射复用。

### F2: render-to-texture 特殊形式

**方案选择:**
不扩展 `define-texture`（其语义是定义一个纹理引用），而是新增特殊形式 `render-to-texture`，它：
1. 评估 body 中的表达式（可能包含 `apply-shader!`）
2. 将得到的 GraphicObject 渲染到离屏 canvas
3. 将渲染结果像素数据打包为 `TextureBox`
4. 返回纹理值（可被 `define` 绑定）

**改动:**

1. `texture_builtins.cpp` 新增特殊形式处理 `render-to-texture`:
   ```pml
   (define grass-diamond
     (render-to-texture 300 150
       (apply-shader! (diamond 150 75 300 150) grass-shader)))
   ```

2. 需要调用 PMLContext 中已注册的 Backend 进行离屏渲染
3. `PMLRuntime` 需要有离屏渲染能力——通过创建临时的 `SkSurface`（`SkSurface::MakeRasterN32Premul`）

**实现要点:**
- 宽高参数必须为整数（或计算后能得出）
- 渲染使用完整的 PML 渲染管线（包括 blend mode, opacity 等）
- 结果纹理可以在后续 `texture-map` 中使用
- 纹理缓存：相同 hash 的 shader+形状+尺寸避免重复渲染

**测试:**
```pml
(define grass-shader
  (noise-fractal :seed 42))
(define grass-tex
  (render-to-texture 300 150
    (apply-shader! (rect 0 0 300 150) grass-shader)))
;; grass-tex 可用于 texture-map
```

---

## Phase 4: 混合模式在 add/group 层面（P1，约 1-2 小时）

> 当前 `:blend-mode` 仅作为形状参数，需要在组合层面设置。

### F6: add/group 关键字参数

**现状:** `GraphicObject` 已有 `blend_mode` 字段（`objects.h:82`）。`add` 和 `group` 在 evaluator 中构造或组合 GraphicObject。

**改动:**
1. `builtins_graphics.cpp` 中的 `add` 和 `group` 特殊形式处理：
   - 接受 `:blend-mode` 关键字参数
   - 如果参数存在，将 `blend_mode` 设置到结果的 GraphicObject 上
   - 对于 `add`：单个 shape 被包装成 Group 时，blend_mode 设置在 group 层
   - 对于 `group`：已有的 `build_group()` 或 `make_group()` 函数扩展 blend_mode 参数

2. `skia_backend_draw.cpp` 的 Group 渲染部分（约 500-520 行附近的循环）：
   - 检查 group 自身的 `blend_mode`，如果有值，为整个 group 的渲染创建带 blend mode 的 `SkPaint`，通过 `SkCanvas::saveLayer` 实现

**实现要点:**
- Group 自有的 blend_mode 应用于整个 group 的合成，不同于子元素各自 blend
- 使用 `SkCanvas::saveLayer(nullptr, &paint)` + `restore()` 实现 group 层混合

**测试:**
```pml
;; 两个着色器结果通过 multiply 混合
(add (group
       (apply-shader! (rect 0 0 100 100) (noise-fractal :seed 1))
       (apply-shader! (rect 0 0 100 100) (noise-fractal :seed 2)))
     :blend-mode :multiply)
```

---

## Phase 5: 裁剪与蒙版（P2，约 4-5 小时）

> 用形状 A 裁剪形状 B 的渲染结果，实现区域合成过渡。

### F5: with-clip 特殊形式

**新增文件:**
- `src/pml/evaluator/clip_builtins.cpp` — `with-clip` 特殊形式

**语法:**
```pml
(with-clip clip-shape body-expression ...)
```

**实现思路:**
1. `with-clip` 是一个特殊形式，它：
   - 评估 `clip-shape` → 得到一个 GraphicObject（裁剪路径）
   - 评估 `body-expression` → 得到一个 GraphicObject（被裁剪内容）
   - 返回一个新的 Group GraphicObject，其 metadata 中标记 `"clip_path": <clip-shape>`

2. Skia 后端 `draw_group()` 识别 `"clip_path"` metadata：
   - 使用 `canvas->save()`
   - 将裁剪路径绘制到 `canvas->clipPath()`（使用 `SkPath`）
   - 渲染被裁剪内容
   - `canvas->restore()`

**补充:**
- 还需要一个 `invert-clip` 或类似机制支持裁剪补集（用 `SkClipOp::kDifference`）
- 裁剪路径可以使用任何形状（circle, rect, polygon）
- 复杂裁剪路径可能需要先转为 SkPath

**测试:**
```pml
;; 用噪波多边形裁剪矩形
(define noise-boundary (polygon ...noisy-vertices...))
(add (with-clip noise-boundary
       (apply-shader! (rect 0 0 300 150) grass-shader)))
```

---

## Phase 6: 透视纹理映射（P3，约 3-4 小时）

> 将矩形纹理映射到任意四边形（等距 tile 侧面）。

### F10: texture-map :perspective-correction

**现状:** `texture-map` 在 `texture_builtins.cpp` 中由 `register_texture_builtins()` 注册。当前实现检查 `:uv-vertices` 参数，在 `:explicit` 模式下使用这些 UV 坐标。但 Skia 的默认 `SkShader` 使用仿射变换（3×2 矩阵），不支持透视校正。

**方案:**
1. 扩展 `texture-map` 接受 `:perspective-correction` 参数（boolean，默认 false）
2. 当启用时，计算透视变换矩阵（3×3），使用 `SkMatrix::setPersp()` 或直接构造
3. 通过 `SkShader::makeWithLocalMatrix()` 应用该矩阵
4. 如果 Skia 版本不支持硬件透视插值，降级为软件逐片段渲染（或使用 `SkImage::makeShader` + `SkSamplingOptions`）

**实现细节:**
- 给定 4 个目标点 (dst0, dst1, dst2, dst3) 和 4 个 UV 点 (0,0), (1,0), (1,1), (0,1)
- 使用 `SkMatrix::setPolyToPoly(dst, src, 4)` 计算单应性矩阵（perspective 3×3）
- 如果该函数返回 false，降级为仿射映射（会丢失透视效果但不会崩溃）

**测试:**
```pml
(texture-map (polygon '((0 0) (100 0) (120 100) (-20 100)))  ;; 梯形
  :source my-texture
  :mode :explicit
  :uv-vertices '((0 0) (1 0) (1 1) (0 1))
  :perspective-correction true)
```

---

## Phase 7: 按名称检索对象（P3，约 4-5 小时）

> 在场景合成中引用之前添加的对象。

### F9: 名称注册表

**新增文件:**
- `src/pml/graphics/name_registry.h` — 名称→对象映射

**设计:**

```cpp
class NameRegistry {
    std::unordered_map<std::string, GraphicObject> objects_;
public:
    void register_object(const std::string& name, GraphicObject obj);
    std::optional<GraphicObject> find(const std::string& name) const;
    bool remove(const std::string& name);
    void replace(const std::string& name, GraphicObject obj);
    void clear();
};
```

**PMLContext 集成** (`pml_context.h`):
- 新增 `NameRegistry name_registry` 成员
- 在 `reset()` 中调用 `name_registry.clear()`

**新 builtin 函数**（注册在 `builtins_graphics.cpp` 或新文件 `name_builtins.cpp`）:

```pml
(name "my-shape")      ;; 给 GraphicObject 命名 → 返回带了 name metadata 的 object
(find "my-shape")      ;; 按名称查找对象 → GraphicObject 或 nil
(replace "my-shape" new-obj)  ;; 替换对象
(remove "my-shape")    ;; 删除对象
(set-property "my-shape" :opacity 0.5)  ;; 修改属性
```

**渲染管线:**
- `add` 时如果对象有 name metadata，注册到 NameRegistry
- `render` 前将所有注册对象渲染到 canvas
- `find` 返回注册的 GraphicObject（只读或可修改的副本？推荐返回副本以维护不可变性）

**实现要点:**
- 名称在 `add` 时通过 metadata 传递
- `set-property` 返回新对象（immutable pattern）
- `find` 返回的 GraphicObject 可被进一步修改（`with_fill()` 等）

**测试:**
```pml
(add (name "tile-0-3") (rect 0 0 100 100 :fill "red"))
(set-property "tile-0-3" :fill "blue")

(let ((obj (find "tile-0-3")))
  (if obj
    (add (with_fill obj "green"))
    (add (rect 0 0 10 10 :fill "gray"))))
```

---

## 优先级总表

| Phase | 功能 | 实现复杂度 | 依赖 | 对地形过渡影响 | 预估工时 |
|-------|------|-----------|------|--------------|---------|
| **Phase 0** | F7 (mod/rem) + F8 (位运算) | 极低 | 无 | ⭐⭐⭐ | 1h |
| **Phase 0** | F6 (blend mode on group) | 低 | 无 | ⭐⭐ | 1-2h |
| **Phase 1** | F1 (世界坐标 shader) | 中 | 无 | ⭐⭐⭐⭐⭐ | 3h |
| **Phase 2** | F4 (渐变填充) | 中 | 无 | ⭐⭐⭐ | 3-4h |
| **Phase 3** | F2 (shader→texture) | 中 | Phase 1 可选 | ⭐⭐⭐⭐⭐ | 2-3h |
| **Phase 4** | F5 (裁剪/蒙版) | 高 | 无 | ⭐⭐⭐⭐ | 4-5h |
| **Phase 5** | F10 (透视纹理映射) | 中 | 无 | ⭐⭐ | 3-4h |
| **Phase 6** | F9 (按名称检索) | 高 | 无 | ⭐ | 4-5h |

**推荐实施顺序:**
Phase 0 (P0, 2h) → Phase 1 (P0, 3h) → Phase 2 (P1, 4h) → Phase 3 (P1, 3h) → Phase 4 (P2, 5h) → Phase 5 (P3, 4h) → Phase 6 (P3, 5h)

总计预估: 约 26 小时

## 向后兼容性

所有功能通过新增关键字参数或新增特殊形式实现，不影响现有 PML 代码。默认行为保持不变：

- `apply-shader!` 不带 `:coordinate-space` → `:local`（现有行为）
- `add`/`group` 不带 `:blend-mode` → 无 group 层混合（现有行为）
- `texture-map` 不带 `:perspective-correction` → 仿射映射（现有行为）
- 不修改 `:fill` 语义：`:fill-gradient` 是新参数，不影响现有 `:fill`
- F7 `mod`/`rem` 是新别名，不影响已有的 `modulo`/`remainder`
