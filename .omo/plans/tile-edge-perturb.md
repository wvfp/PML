# Tile Edge Perturbation — PML 多边形顶点扰动 + 圆角

## TL;DR

> **Quick Summary**: 在 PML 的 `polygon` 形状上添加边缘顶点扰动和圆角功能，让等距瓦片轮廓不再方方正正，产生自然的不规则感。通过 `perturb-polygon` 内置函数 + `polygon` 直接 kwarg 双层 API 实现，用户拿到扰动后的顶点列表自由构建侧面。
>
> **Deliverables**:
> - CPU 2D Perlin 噪声实现 (`graphics/perlin_noise.h/.cpp`)
> - 多边形扰动算法 (`graphics/polygon_perturb.h/.cpp`)
> - `perturb-polygon` PML 内置函数（返回嵌套顶点列表）
> - `polygon` 形状支持 `:edge-noise` 等 kwarg
> - GTest 单元测试 + PML smoke 测试
> - API 文档 (`docs/api/11-edge-perturb.md`)
>
> **Estimated Effort**: Medium (~1 week)
> **Parallel Execution**: YES — 4 waves
> **Critical Path**: Perlin noise → perturb algorithm → perturb-polygon builtin → polygon kwarg → tests

---

## Context

### Original Request
用户生成 2.5D 等距瓦片时，所有瓦片是完美菱形/正方形，缺乏自然感。希望瓦片外部轮廓不规则，但拼接边保持笔直。

### Interview Summary
**Key Discussions**:
- 三层设计（Layer 0: `perturb-polygon`, Layer 1: polygon kwarg, Layer 2: extrude-tile）但用户选择 `perturb-polygon + 自己画`，侧面完全由 PML 自定义
- 按边独立控制，向量传参（标量=统一，向量=按边）
- Catmull-Rom 样条插值边细分
- 圆角用弧线（非直线切角）
- 世界坐标 Perlin 噪声确保相邻瓦片共享边一致
- 扰动在构建时发生，存在 metadata 而非 ParamKey

**Research Findings**:
- ParamKey 枚举紧凑（24 成员），不应膨胀
- rough-style 的参数存在 GraphicObject.metadata，mode 15
- 现有噪声 `noise-fractal` 是 GPU SkSL，CPU 侧无噪声实现
- shape_builtins.cpp 已解析 polygon 的 kwarg

### Metis Review
**Identified Gaps** (addressed):
- 8 个设计决策全部通过 Question 确认（返回类型、扰动时机、噪声方案、参数存储、输出格式、插值方案、负噪声、侧面颜色）
- `extrude-tile` 从 C++ 层移除，用户自行构建
- Catmull-Rom 插值（用户选择）需实现
- 负噪声允许但有保护（clamp 到最小边长的 1/2）

---

## Work Objectives

### Core Objective
为 PML 的 `polygon` 形状添加顶点级边缘扰动和圆角功能，支持按边独立控制、世界坐标一致性的噪声位移、Catmull-Rom 样条插值，全部通过 TDD 验证。

### Concrete Deliverables
- `src/pml/graphics/perlin_noise.h` / `.cpp` — 确定性 2D Perlin 噪声
- `src/pml/graphics/polygon_perturb.h` / `.cpp` — 多边形扰动核心算法
- `src/pml/evaluator/perturb_builtins.cpp` / `.h` — `perturb-polygon` 内置函数
- `src/pml/evaluator/shape_builtins.cpp` — `polygon` 新增 kwarg 解析
- `tests/polygon_perturb_test.cpp` — GTest 单元测试
- `tests/builtins_smoke.cpp` — PML smoke 测试（已有文件追加）
- `docs/api/20-edge-perturb.md` — API 文档
- 示例 `.pml` 文件展示用法

### Definition of Done
- [ ] `cmake --build --preset debug` 编译通过
- [ ] `.\build\debug\tests\Debug\pml-builtins-smoke.exe` 全部通过
- [ ] GTest 单元测试全部通过
- [ ] 零噪声恒等回归：原始 polygon 输出像素不变
- [ ] 示例 .pml 文件渲染出有效 PNG

### Must Have
- `perturb-polygon` 返回嵌套列表 `((x y) ...)`，可直接传入 `polygon`
- 参数支持标量（统一值）和向量（按边独立）：`:edge-noise 0.15` / `:edge-noise (vector 0.15 0.0 0.12 0.0)`
- 边索引规则：边 i = 顶点[i] → 顶点[(i+1) % n]
- Catmull-Rom 样条插值细分边
- 圆角用弧线（三次贝塞尔逼近），半径自动 clamp
- 负噪声允许，位移 <= min(相邻边)/2
- 世界坐标 Perlin 噪声保证确定性，相同点得到相同位移
- 参数存 metadata，不修改 ParamKey 枚举
- TDD：先写 GTest 再实现

### Must NOT Have (Guardrails)
- 不改 ParamKey 枚举（参数存 metadata）
- 不改渲染后端（扰动在构建时完成，后端看到的已经是扰动后的顶点）
- 不改现有 tilemap/tileset 系统
- 不做 `extrude-tile`（用户自己用 `perturb-polygon` 返回值构建侧面）
- 不做 shader displacement（留待未来）
- 不做 GPU 噪声（CPU 自实现 Perlin）

---

## Verification Strategy (MANDATORY)

> **ZERO HUMAN INTERVENTION** — ALL verification is agent-executed.

### Test Decision
- **Infrastructure exists**: YES
- **Automated tests**: TDD
- **Framework**: GTest (`tests/polygon_perturb_test.cpp`) + PML smoke (`tests/builtins_smoke.cpp`)
- **TDD**: 先写 GTest 测试用例（已知输入→已知输出），再实现算法

### QA Policy
Every task MUST include agent-executed QA scenarios.
- **Algorithm tests**: GTest assertions on known input/output
- **Integration tests**: PML smoke test via `builtins_smoke.exe`
- **Visual tests**: Render .pml to PNG, verify file exists and has content

---

## Execution Strategy

```
Wave 1 (Foundation — 3 tasks):
├── Task 1: CPU 2D Perlin noise [quick]
├── Task 2: Polygon perturb algorithm [deep]
└── Task 3: GTest for noise + perturb [unspecified-high]

Wave 2 (PML integration — 4 tasks):
├── Task 4: perturb-polygon builtin [unspecified-high]
├── Task 5: polygon kwarg parsing [unspecified-high]
├── Task 6: PML smoke tests [unspecified-high]
└── Task 7: API docs [writing]

Wave FINAL (Verification — 4 parallel reviews):
├── Task F1: Plan compliance audit (oracle)
├── Task F2: Code quality review (unspecified-high)
├── Task F3: Real manual QA (unspecified-high)
└── Task F4: Scope fidelity check (deep)
→ Present results → Get explicit user okay

Critical Path: Task 1 → Task 2 → Task 4 → Task F1-F4 → user okay
Parallel Speedup: ~40% faster than sequential
Max Concurrent: 4 (Wave 1)
```

### Dependency Matrix
- **1**: - → 2, 3
- **2**: 1 → 4, 5, 6
- **3**: 1 → (no downstream, runs in parallel with Wave 2)
- **4**: 2 → 7
- **5**: 2 → 7
- **6**: 2 → F3
- **7**: 4, 5 → F4

### Agent Dispatch Summary
- **Wave 1**: 3 tasks — T1 → `quick`, T2 → `deep`, T3 → `unspecified-high`
- **Wave 2**: 4 tasks — T4-T6 → `unspecified-high`, T7 → `writing`
- **FINAL**: 4 tasks — F1 → `oracle`, F2 → `unspecified-high`, F3 → `unspecified-high`, F4 → `deep`

---

## TODOs

- [x] 1. **实现 CPU 2D Perlin 噪声** — `src/pml/graphics/perlin_noise.h/.cpp`

  **What to do**:
  - 新建 `perlin_noise.h`，声明 `class PerlinNoise2D`
  - `PerlinNoise2D(uint32_t seed)` 构造，`double noise(double x, double y)` 返回 [-1, 1]
  - 标准 Perlin 噪声算法：梯度表→点积→双线性插值→fade 曲线
  - 支持八度叠加：`double fractal(double x, double y, int octaves, double persistence, double lacunarity)`
  - 纯确定性：相同 seed + 相同坐标 → 相同输出
  - 头文件 `#pragma once`，放 `namespace pml` 内

  **Must NOT do**:
  - 不依赖 GPU/Skia 噪声
  - 不使用全局/静态 RNG 状态
  - 不使用 `std::random_device` 或 `std::mt19937`

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: 这是纯粹的数学算法实现，无外部依赖，代码量约 100 行
  - **Skills**: none needed

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 2, 3)
  - **Blocks**: Tasks 2, 3
  - **Blocked By**: None (foundation task)

  **References**:
  - `https://mrl.cs.nyu.edu/~perlin/noise/` — Ken Perlin 原始论文
  - `src/pml/graphics/rough.h` — 现有扰动代码，查看其数学模式
  - `src/pml/evaluator/shader_builtins.cpp` — 查看 `noise-fractal` 的参数接口风格（用于 PML 层一致性）

  **Acceptance Criteria**:

  **GTest (TDD — write test first):**
  - [ ] `tests/polygon_perturb_test.cpp` 创建，包含：
  - [ ] `PerlinNoise2D(42).noise(0.0, 0.0) == 0.0` (Perlin 噪声原点为 0)
  - [ ] `PerlinNoise2D(42).noise(1.0, 0.0)` 返回确定值（硬编码期望值）
  - [ ] 第二次相同调用返回完全相同结果
  - [ ] 不同 seed 返回不同值
  - [ ] `fractal(0.5, 0.5, 4, 0.5, 2.0)` 不崩溃且在 [-1, 1] 范围内
  - [ ] 1000 次随机坐标调用的均值 ≈ 0（统计检验）

  **Build:**
  - [ ] `cmake --build --preset debug --target polygon_perturb_test` 通过

  **QA Scenarios:**

  ```
  Scenario: 零噪声恒等
    Tool: Bash (GTest)
    Preconditions: GTest binary compiled
    Steps:
      1. 创建噪声对象 PerlinNoise2D(seed=0)
      2. 调用 noise(0,0) → 返回 0.0
      3. 调用 noise(1,0) → 返回确定值
    Expected Result: 返回值与预期一致
    Evidence: .omo/evidence/task-1-zero-return.txt

  Scenario: 确定性保证
    Tool: Bash (GTest)
    Preconditions: GTest binary compiled
    Steps:
      1. 创建 PerlinNoise2D(42)
      2. 调用 noise(3.14, 2.71) 两次
      3. 断言两次结果完全一致 (memcmp)
    Expected Result: 两次结果字节一致
    Evidence: .omo/evidence/task-1-deterministic.txt

  Scenario: 八度叠加不崩溃
    Tool: Bash (GTest)
    Preconditions: GTest binary compiled
    Steps:
      1. fractal(0.5, 0.5, 8, 0.5, 2.0)
      2. 断言结果在 [-1, 1] 范围内
    Expected Result: 返回值在有效范围内
    Evidence: .omo/evidence/task-1-fractal.txt
  ```

  **Commit**: YES
  - Message: `feat(graphics): add deterministic 2D Perlin noise`
  - Files: `src/pml/graphics/perlin_noise.h`, `src/pml/graphics/perlin_noise.cpp`, `tests/polygon_perturb_test.cpp`

---

- [x] 2. **实现多边形扰动核心算法** — `src/pml/graphics/polygon_perturb.h/.cpp`

  **What to do**:
  - 新建 `polygon_perturb.h`，声明 `PerturbConfig` 结构体和 `perturb_polygon()` 函数
  - `PerturbConfig` 字段：`edge_noise`(vector<double>), `edge_mask`(vector<bool>), `edge_subdivisions`(vector<int>), `corner_radius`(vector<double>), `corner_mask`(vector<bool>), `seed`(uint32_t)
  - `perturb_polygon()` 输入原始顶点列表 + `PerturbConfig` + `PerlinNoise2D`，返回扰动后顶点列表
  - 算法流程：
    1. 对每条边：根据 `edge_mask[i]` 决定是否扰动
    2. 如果扰动：按 `edge_subdivisions[i]` 细分边，用 Catmull-Rom 样条插值细分点
    3. 每个细分点沿法线方向用 Perlin 噪声位移，幅度 `edge_noise[i] * 边长`
    4. 如果不扰动：保持原始直线
    5. 对每个角：根据 `corner_mask[i]` 决定是否圆角
    6. 如果圆角：用三次贝塞尔曲线替代尖角，半径 `corner_radius[i]`，自动 clamp 到 min(相邻边)/2
  - 世界坐标一致性：位移量由 Perlin 噪声基于**世界坐标**计算，非顶点索引
  - 标量参数自动扩展为向量（`edge_noise=0.15` → 4 条边各 0.15）
  - 边索引规则：边 i = vertex[i] → vertex[(i+1) % n]

  **Must NOT do**:
  - 不修改渲染后端
  - 不自相交检查（文档说明为"用户责任"，debug 构建加断言）
  - 不做 Catmull-Rom 以外的插值方案

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: 核心算法，涉及 Catmull-Rom 样条、法线计算、贝塞尔逼近圆角等多个数学子问题，需要整体设计
  - **Skills**: none needed

  **Parallelization**:
  - **Can Run In Parallel**: NO (depends on Task 1)
  - **Parallel Group**: Wave 1 (after Task 1)
  - **Blocks**: Tasks 4, 5, 6
  - **Blocked By**: Task 1

  **References**:
  - `src/pml/graphics/perlin_noise.h` — 本波 Task 1 的输出
  - `src/pml/graphics/objects.h` — GraphicObject 结构体
  - `src/pml/graphics/params.h` — ParamKey::points 如何存储多边形顶点
  - `https://en.wikipedia.org/wiki/Catmull–Rom_spline` — Catmull-Rom 样条公式
  - `src/pml/graphics/rough.cpp` — 现有线段扰动算法参考
  - `src/pml/graphics/rough.h` — RoughStyleParams 结构体风格参考

  **Acceptance Criteria**:

  **GTest (TDD — write test first):**
  - [ ] 4 顶点正方形 (0,0)-(10,0)-(10,10)-(0,10), `edge_noise=0` → 返回原始 4 个顶点
  - [ ] 带 `edge_subdivisions=3` 但不扰动 → 返回 (3*4+4)=16 个 Catmull-Rom 细分点
  - [ ] 带 `edge_noise=0.2` → 输出与原始顶点不同，且确定性（相同输入→相同输出）
  - [ ] `edge_mask=(#f #t #f #t)` → 第 1 和 3 条边被细分+扰动，第 0 和 2 条边保持直线
  - [ ] `corner_radius=2` → 输出顶点数多于输入，尖角被弧线替代
  - [ ] 半径 clamp：`corner_radius=999` 自动限制到合理值，不崩溃
  - [ ] 3 顶点三角形 — 正常处理（最小多边形）
  - [ ] 5+ 顶点多边形 — 正常处理
  - [ ] 退化输入（2 顶点） → 应返回错误

  **QA Scenarios:**

  ```
  Scenario: 零噪声恒等回归
    Tool: Bash (GTest)
    Preconditions: GTest binary compiled
    Steps:
      1. perturbe_polygon(square, noise=0) → 返回原始 4 顶点或 Catmull-Rom 细分点
      2. 如果 edge_subdivisions=0 → 像素级还原
    Expected Result: 扰动=0 时形状不变
    Evidence: .omo/evidence/task-2-zero-noise.txt

  Scenario: 非零扰动 + 确定性
    Tool: Bash (GTest)
    Preconditions: GTest binary compiled
    Steps:
      1. perturbe_polygon(square, noise=0.2, subdivisions=3, seed=42)
      2. 重复一次完全相同调用
      3. 断言两次返回的顶点列表元素相同
      4. 确认顶点不同于原始正方形顶点
    Expected Result: 两次结果一致，且形状有变化
    Evidence: .omo/evidence/task-2-deterministic.txt

  Scenario: 掩码控制
    Tool: Bash (GTest)
    Preconditions: GTest binary compiled
    Steps:
      1. perturbe_polygon(square, noise=0.2, mask=(#f #t #f #t))
      2. 验证边 1 和 3 的顶点有位移，边 0 和 2 保持直线
    Expected Result: 仅被掩码允许的边发生位移
    Evidence: .omo/evidence/task-2-mask.txt
  ```

  **Commit**: YES (with Task 1)
  - Message: `feat(graphics): add 2D Perlin noise and polygon perturbation`
  - Files: `src/pml/graphics/polygon_perturb.h`, `src/pml/graphics/polygon_perturb.cpp`

---

- [x] 3. **GTest 单元测试：噪声 + 扰动** — `tests/polygon_perturb_test.cpp`

  **What to do**:
  - 确保 `tests/polygon_perturb_test.cpp` 包含完整的单元测试套件
  - 测试组织：`TEST(PerlinNoise2D, ...)` 和 `TEST(PolygonPerturb, ...)` 两组
  - Perlin 测试：
    - 原点为零、确定性、不同 seed 不同值、八度叠加范围、统计均匀性
    - 浮点顺序敏感性：x=3.14 和 x=3.14+1e-15 应返回接近值（连续）
  - 扰动测试：
    - 零噪声恒等、非零噪声改变、掩码控制、圆角、半径 clamp
    - Catmull-Rom 细分点在线段上均匀分布（验证插值正确性）
    - 5 边形、三角形、8 边形等不同顶点数
  - 将测试目标添加到 `tests/CMakeLists.txt`

  **Must NOT do**:
  - 不测试 PML 解析层（那是 smoke test 的事）
  - 不用浮点 == 比较，用 `EXPECT_NEAR` 或 `EXPECT_FLOAT_EQ`

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
    - Reason: 需要全面覆盖边缘情况和数值精度，但逻辑清晰
  - **Skills**: none needed

  **Parallelization**:
  - **Can Run In Parallel**: YES (after Task 1, test the API)
  - **Parallel Group**: Wave 1 (with Tasks 1, 2)
  - **Blocks**: Nothing (verification layer)
  - **Blocked By**: Task 1

  **References**:
  - `tests/test_graphics.cpp` — 现有 GTest 测试风格
  - `tests/test_helpers.h` — 测试辅助函数
  - `tests/CMakeLists.txt` — 如何添加测试 target

  **Acceptance Criteria**: (all covered in QA scenarios above)
  - [ ] 所有测试通过
  - [ ] 覆盖率：Perlin 噪声 + 扰动核心路径 + 边界情况

  **QA Scenarios:**

  ```
  Scenario: 全部运行通过
    Tool: Bash
    Preconditions: cmake --build --preset debug 成功
    Steps:
      1. tests\Debug\polygon_perturb_test.exe
      2. 捕获退出码和输出
    Expected Result: 退出码 0，输出无 FAILED
    Evidence: .omo/evidence/task-3-all-pass.txt
  ```

  **Commit**: YES
  - Message: `test(graphics): add GTest for noise and perturb algorithms`
  - Files: `tests/polygon_perturb_test.cpp`, `tests/CMakeLists.txt`

---

- [x] 4. **实现 `perturb-polygon` PML 内置函数** — `src/pml/evaluator/perturb_builtins.h/.cpp`

  **What to do**:
  - 新建 `perturb_builtins.h` / `.cpp`
  - 注册 `perturb-polygon` 内置函数
  - 位置参数：`vertices` — 顶点列表 `((x y) ...)`
  - Kwargs：`:edge-noise`, `:edge-mask`, `:edge-subdivisions`, `:corner-radius`, `:corner-mask`, `:edge-seed`
  - 解析规则：
    - 如果 kwarg 值是 vector → 按边使用
    - 如果 kwarg 值是 number/bool → 自动扩展为所有边相同的向量
    - `:edge-seed` 是 int（默认 0）
  - 内部调用 `PerturbConfig` + `PerlinNoise2D` + `perturb_polygon()`
  - 返回嵌套列表 `((x y) (x y) ...)`，每个元素是 2 元素 list
  - 注册函数 `register_perturb_builtins(env)`，在 `api.cpp` 中调用
  - 处理错误：顶点数 < 3 给出明确错误信息

  **Must NOT do**:
  - 不渲染任何图形（纯坐标变换）
  - 不修改 `polygon` 行为
  - 不处理 2 顶点以下的多边形

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
    - Reason: PML 内置函数注册类似 shader_builtins.cpp 模式，需理解 kwargs 解析、Value 类型转换
  - **Skills**: none needed

  **Parallelization**:
  - **Can Run In Parallel**: NO (depends on Task 2)
  - **Parallel Group**: Wave 2 (with Tasks 5, 6, 7)
  - **Blocks**: Nothing (users call it from PML)
  - **Blocked By**: Task 2

  **References**:
  - `src/pml/evaluator/shader_builtins.cpp` — 内置函数注册模式（kwargs 解析、返回 Value 类型）
  - `src/pml/evaluator/tilemap_builtins.cpp` — 查看 `value_to_expr` / `eval_value_as_expr` 模式
  - `src/pml/evaluator/shape_builtins.cpp` — 查看 `builtin_polygon` 如何解析顶点列表
  - `src/pml/api/api.cpp` — 如何调用 `register_*` 函数
  - `src/pml/graphics/polygon_perturb.h` — `PerturbConfig` 和 `perturb_polygon()` 签名

  **Acceptance Criteria**:

  **PML Smoke Test:**
  - [ ] `(perturb-polygon (list (list 0 0) (list 10 0) (list 10 10) (list 0 10)) :edge-noise 0)` → 返回 4 个原始顶点（无细分时）
  - [ ] `(perturb-polygon (list (list 0 0) (list 10 0)) ...)` → 错误：至少 3 个顶点
  - [ ] `(perturb-polygon ... :edge-noise (vector 0.1 0.0 0.1 0.0))` → 按边独立扰动
  - [ ] 两次相同调用返回相同结果（确定性）

  **QA Scenarios:**

  ```
  Scenario: PML 调用 perturb-polygon
    Tool: Bash
    Preconditions: pml-builtins-smoke.exe 包含测试
    Steps:
      1. builtins_smoke.exe 运行包含 (perturb-polygon ...) 的测试
      2. 检查返回值是嵌套列表
    Expected Result: 返回格式正确的嵌套列表
    Evidence: .omo/evidence/task-4-builtin-return.txt

  Scenario: 按边独立控制
    Tool: Bash
    Preconditions: pml-builtins-smoke.exe 编译
    Steps:
      1. 运行 smoke test，传入 edge-noise=(vector 0.1 0.0 0.1 0.0)
      2. 确认只有边 0 和 2 的顶点有位移
    Expected Result: 掩码生效
    Evidence: .omo/evidence/task-4-per-edge.txt

  Scenario: 错误处理
    Tool: Bash
    Preconditions: pml-builtins-smoke.exe 编译
    Steps:
      1. 运行传入 2 个顶点的测试
      2. 检查是否抛出合理错误
    Expected Result: 错误信息包含 "至少需要 3 个顶点"
    Evidence: .omo/evidence/task-4-error.txt
  ```

  **Commit**: YES
  - Message: `feat(evaluator): add perturb-polygon builtin`
  - Files: `src/pml/evaluator/perturb_builtins.h`, `src/pml/evaluator/perturb_builtins.cpp`, `src/pml/api/api.cpp`

---

- [x] 5. **polygon 形状直接支持扰动 kwarg** — `src/pml/evaluator/shape_builtins.cpp`

  **What to do**:
  - 在 `builtin_polygon` 中解析新增 kwargs：`:edge-noise`, `:edge-mask`, `:edge-subdivisions`, `:corner-radius`, `:corner-mask`, `:edge-seed`
  - 如果检测到 `:edge-noise` 存在且 != 0：
    1. 构建 `PerturbConfig`
    2. 调用 `perturb_polygon()` 获取扰动后顶点
    3. 将扰动后的顶点写入 `ParamKey::points`
    4. 将原始 kwarg 值存入 `metadata`（保持参数可见性）
  - 如果没有 `:edge-noise` 或 =0：行为完全不变（零回归）
  - `:edge-seed` 不在 polygon kwarg 中时，用 `std::hash` 从顶点列表生成种子，保证瓦片级确定性

  **Must NOT do**:
  - 不改 ParamKey 枚举
  - 不改 GraphicObject 结构体
  - 不改渲染后端
  - 不影响非 polygon 形状（circle, rect 等不解析这些 kwarg）

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
    - Reason: 需要在现有 shape_builtins.cpp 的 kwarg 解析链中插入新逻辑，需理解 Params、metadata、GraphicObject 的交互
  - **Skills**: none needed

  **Parallelization**:
  - **Can Run In Parallel**: NO (depends on Task 2)
  - **Parallel Group**: Wave 2 (with Tasks 4, 6, 7)
  - **Blocks**: Task 7
  - **Blocked By**: Task 2

  **References**:
  - `src/pml/evaluator/shape_builtins.cpp` — 现有 `builtin_polygon` 实现
  - `src/pml/graphics/params.h` — `ParamKey::points`
  - `src/pml/graphics/objects.h` — `GraphicObject::metadata`
  - `src/pml/graphics/polygon_perturb.h` — `perturb_polygon()` 和 `PerturbConfig`

  **Acceptance Criteria**:

  **PML Smoke Test:**
  - [ ] 现有 polygon 无一使用新 kwarg → 完全不变（回归测试）
  - [ ] `(polygon pts :edge-noise 0)` → 与不加 kwarg 的 polygon 输出一致
  - [ ] `(polygon pts :edge-noise 0.2 :edge-mask (vector #t #f #t #f))` → 扰动生效
  - [ ] 渲染含扰动 polygon 的 .pml → 输出有效 PNG

  **QA Scenarios:**

  ```
  Scenario: 回归测试 — 不加 kwarg 的 polygon 不变
    Tool: Bash
    Preconditions: 已有渲染测试 .pml 文件
    Steps:
      1. 用旧版 pml.exe 渲染一个 polygon-only 的 .pml → out-before.png
      2. 用新版 pml.exe 渲染同一个 .pml → out-after.png
      3. 二进制比较两个 PNG
    Expected Result: 两个 PNG 字节一致（无变化）
    Evidence: .omo/evidence/task-5-regression.txt

  Scenario: 带扰动的 polygon 渲染
    Tool: Bash
    Preconditions: pml.exe 编译
    Steps:
      1. 创建 demo.pml：(polygon pts :edge-noise 0.15 :edge-subdivisions 3 :edge-seed 42 :fill "#5DAE3B")
      2. 渲染 → 检查 PNG 文件存在且 > 1KB
      3. 用不同 seed 渲染第二个 PNG，确认形状不同
    Expected Result: 两个 PNG 形状不一致（扰动生效）
    Evidence: .omo/evidence/task-5-perturb-render.png
  ```

  **Commit**: YES (with Task 6)
  - Message: `feat(evaluator): add edge-noise kwarg on polygon + smoke tests`
  - Files: `src/pml/evaluator/shape_builtins.cpp`

---

- [x] 6. **PML Smoke 测试** — `tests/builtins_smoke.cpp`

  **What to do**:
  - 在 `tests/builtins_smoke.cpp` 中追加以下烟雾测试用例：
    1. **perturb-polygon 零噪声**：`(perturb-polygon pts :edge-noise 0)` → 返回匹配 `pts` 的列表
    2. **perturb-polygon 确定性**：两次调用 → 结果相同（`equal?` 断言）
    3. **perturb-polygon 按边掩码**：用 `:edge-mask (vector #t #f ...)` → 确认
    4. **perturb-polygon 圆角**：`(perturb-polygon pts :corner-radius 5 :edge-subdivisions 0)` → 顶点数增加，角变弧线
    5. **perturb-polygon 标量扩展**：`:edge-noise 0.2` 对标 `vector` 版
    6. **perturb-polygon 错误**：2 顶点 → 预期错误
    7. **polygon 加 kwarg 渲染**：调用 `(polygon pts :edge-noise 0.15 :edge-seed 42)` → 不崩溃
    8. **组合测试**：`(polygon (perturb-polygon pts :edge-noise 0.2 :edge-seed 1) :edge-noise 0.1 :edge-seed 2)` — 内层加外层叠加，确认不冲突
  - 遵循现有 `CHECK` / `CHECK_ERROR` 宏格式

  **Must NOT do**:
  - 不改现有的 smoke test（追加不修改）
  - 不测试渲染像素内容（那是 QA 的事）

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
    - Reason: 需要全面的 PML 用例覆盖，确保边界情况、错误路径、组合场景都测试到
  - **Skills**: none needed

  **Parallelization**:
  - **Can Run In Parallel**: YES (after Task 2)
  - **Parallel Group**: Wave 2 (with Tasks 4, 5, 7)
  - **Blocks**: Nothing
  - **Blocked By**: Task 2

  **References**:
  - `tests/builtins_smoke.cpp` — 现有 smoke test 格式，`CHECK()` / `CHECK_ERROR()` 宏
  - `tests/_smoke_module.pml` — 辅助 PML 模块

  **Acceptance Criteria**:
  - [ ] 所有 8 个新测试用例全部通过
  - [ ] 现有测试全部通过（零回归）

  **QA Scenarios:**

  ```
  Scenario: smoke test 全通过
    Tool: Bash
    Preconditions: cmake --build --preset debug 成功
    Steps:
      1. .\build\debug\tests\Debug\pml-builtins-smoke.exe
      2. 捕获输出
    Expected Result: 全部测试通过，退出码 0
    Evidence: .omo/evidence/task-6-smoke-pass.txt

  Scenario: 确定性验证
    Tool: Bash (smoke test 内)
    Preconditions: smoke test 包含 equal? 断言
    Steps:
      1. 两次调用 perturb-polygon 并用 equal? 比较
    Expected Result: equal? → #t
    Evidence: .omo/evidence/task-6-determinism.txt
  ```

  **Commit**: YES (with Task 5)
  - Files: `tests/builtins_smoke.cpp`

---

- [x] 7. **API 文档** — `docs/api/20-edge-perturb.md`

  **What to do**:
  - 新建 `docs/api/11-edge-perturb.md`
  - 文档结构：
    1. 概述 — 说明功能和用途
    2. `perturb-polygon` — 完整参数表、返回值、示例
    3. polygon `:edge-noise` 等 kwarg — 说明 + 示例
    4. 边索引规则图（菱形、正方形、任意多边形）
    5. 参数类型说明（标量=统一，向量=按边）
    6. 组合示例：`perturb-polygon` + `polygon` + 自己构建侧面
    7. 迁移指南：如何将现有 tile 代码加参数变不规则
  - 更新 `docs/SKILL.md`：添加 mode 17 条目，API 文档列表增加 11-edge-perturb.md

  **Must NOT do**:
  - 不写 extrude-tile（未实现）
  - 不写 shader displacement 相关内容

  **Recommended Agent Profile**:
  - **Category**: `writing`
    - Reason: 纯文档编写，需要清晰的技术写作
  - **Skills**: none needed

  **Parallelization**:
  - **Can Run In Parallel**: YES (after Task 4, 5)
  - **Parallel Group**: Wave 2 (with Tasks 4, 5, 6)
  - **Blocks**: Nothing
  - **Blocked By**: Tasks 4, 5

  **References**:
  - `docs/api/10-tilemap.md` — API doc 风格
  - `docs/SKILL.md` — 需要更新
  - `src/pml/evaluator/perturb_builtins.h` — API 接口签名

  **Acceptance Criteria**:
  - [ ] 文档完整，包含所有参数的说明和类型
  - [ ] 至少 3 个 PML 示例代码块
  - [ ] SKILL.md 已更新

  **QA Scenarios:**

  ```
  Scenario: 文档完整性检查
    Tool: Bash
    Preconditions: 文件已创建
    Steps:
      1. 检查 docs/api/11-edge-perturb.md 存在
      2. 检查是否包含 "perturb-polygon" 章节
      3. 检查 SKILL.md 是否引用了 11-edge-perturb.md
    Expected Result: 文件存在且结构完整
    Evidence: .omo/evidence/task-7-doc-exists.txt
  ```

  **Commit**: YES
  - Message: `docs: add 11-edge-perturb API documentation`
  - Files: `docs/api/11-edge-perturb.md`, `docs/SKILL.md`

---

## Final Verification Wave (MANDATORY)

- [x] F1. **Plan Compliance Audit** — `oracle`
  Read the plan end-to-end. For each "Must Have": verify implementation exists (read file, compile, run command). For each "Must NOT Have": search codebase for forbidden patterns — reject with file:line if found. Check evidence files exist in .omo/evidence/. Compare deliverables against plan.
  Output: `Must Have [N/N] | Must NOT Have [N/N] | Tasks [N/N] | VERDICT: APPROVE/REJECT`

- [x] F2. **Code Quality Review** — `unspecified-high`
  Run `cmake --build --preset debug` + `.\build\debug\tests\Debug\pml-builtins-smoke.exe`. Review all changed files for unused includes, commented-out code, debug printfs, magic numbers. Check AI slop.
  Output: `Build [PASS/FAIL] | Tests [N pass/N fail] | Files [N clean/N issues] | VERDICT`

- [x] F3. **Real Manual QA** — `unspecified-high`
  Start from clean build. Execute EVERY QA scenario from EVERY task — follow exact steps, capture evidence. Test cross-task integration: `(polygon (perturb-polygon ...) :edge-noise ...)`. Test edge cases: zero noise, full mask, 3-gon, degenerate 2-vertex. Save to `.omo/evidence/final-qa/`.
  Output: `Scenarios [N/N pass] | Integration [N/N] | Edge Cases [N tested] | VERDICT`

- [x] F4. **Scope Fidelity Check** — `deep`
  For each task: read "What to do", read actual diff (git log/diff). Verify 1:1 — everything in spec was built (no missing), nothing beyond spec was built (no creep). Check "Must NOT do" compliance. Detect cross-task contamination (Task 4 touching Task 5's files).
  Output: `Tasks [N/N compliant] | Contamination [CLEAN/N issues] | Unaccounted [CLEAN/N files] | VERDICT`

---

## Commit Strategy

- **Task 1-2**: `feat(graphics): add 2D Perlin noise and polygon perturbation`
- **Task 3**: `test(graphics): add GTest for noise and perturb algorithms`
- **Task 4**: `feat(evaluator): add perturb-polygon builtin`
- **Task 5-6**: `feat(evaluator): add edge-noise kwarg on polygon + smoke tests`
- **Task 7**: `docs: add 11-edge-perturb API documentation`

---

## Success Criteria

### Verification Commands
```bash
# Build
cmake --build --preset debug

# Run tests
.\build\debug\tests\Debug\pml-builtins-smoke.exe

# Render test
.\build\debug\bin\Debug\pml.exe examples/edge-perturb/demo.pml -o out/
```

### Final Checklist
- [ ] All "Must Have" implemented
- [ ] All "Must NOT Have" absent
- [ ] All tests pass (GTest + smoke)
- [ ] Zero regression on existing polygon tests
- [ ] API doc written
