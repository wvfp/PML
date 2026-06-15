# PML Phase 8 实现计划 — 骨骼与 IK

## 目标

为 PML 添加骨骼系统（`defskeleton`）和逆向动力学（`ik-solve`），支持 FABRIK/CCD 两种求解算法、皮肤绑定（`bind-skin`）和关节位置查询（`joint-position`），并与动画时间轴集成，实现骨骼驱动的帧动画。

**最终验证**：能通过 PML 代码定义骨骼、执行 IK 求解，配合 `every-frame` 钩子输出骨骼动画 GIF。

---

## 现有基础设施

- `Timeline` 单例 — `frame_hooks` 列表已在 `render_frames()` 中每帧调用
- `GraphicObject` — frozen dataclass，有 `with_transform()` / `with_param()`
- `AffineTransform` — 已有 `translate/rotate/scale/compose` 构造器
- `SPECIAL_FORMS` dict — 在 evaluator.py 中分派，`defskeleton` 需要加入
- `BuiltinProcedure` — `accepts_kwargs=True` 支持关键字参数
- `errors.py` — 需添加 `IKNoSolutionError`
- `repl.py` — `create_global_env()` 中需添加 `register_skeleton(env)`

## 关键设计约束

- `defskeleton` 必须是**特殊形式**（joint 定义中的 `:pos`、`:length` 等关键字不能被当作普通参数求值）
- `SkeletonInstance` 的角度状态必须是**可变的**（IK 求解器直接修改）
- FABRIK 在位置空间操作，求解后需反算角度
- CCD 在角度空间操作，逐关节旋转使末端逼近目标
- `bind-skin` 需要在全局注册表中记录绑定关系，帧渲染时自动更新

---

## 实现步骤

### Step 1：错误类型 `pml/errors.py` (MODIFY)

添加 `IKNoSolutionError`：

```python
class IKNoSolutionError(PMLError):
    """IK solver failed to converge within max iterations."""
```

### Step 2：骨骼数据模型 `pml/skeleton/skeleton.py` (NEW)

```python
@dataclass(frozen=True)
class Joint:
    """Joint definition within a skeleton template."""
    name: str
    pos: tuple[float, float]   # offset from parent joint
    length: float              # bone segment length
    angle: float               # initial angle (radians, relative to parent direction)
    min_angle: float | None    # constraint lower bound (optional)
    max_angle: float | None    # constraint upper bound (optional)

@dataclass(frozen=True)
class SkeletonTemplate:
    """A skeleton blueprint — defines joint topology and initial pose."""
    name: str
    root_params: tuple[str, str]  # parameter names, e.g. ("x", "y")
    joints: tuple[Joint, ...]     # ordered joint chain

class SkeletonInstance:
    """A live skeleton with mutable joint angles."""
    template: SkeletonTemplate
    root_x: float
    root_y: float
    angles: list[float]     # mutable, one per joint
    name: str
    _bindings: list[tuple[int, list[str]]]  # (graphic_id, joint_names)

    def forward_kinematics(self) -> list[tuple[float, float]]:
        """Compute world-space positions for all joints."""

    def joint_world_position(self, joint_name: str) -> tuple[float, float]:
        """Get a specific joint's world position."""

    def clamp_angle(self, index: int, angle: float) -> float:
        """Apply min/max constraints to a joint angle."""
```

**正运动学**（`forward_kinematics`）：
1. 起点 = (root_x, root_y)
2. 对每个 joint i：
   - parent_pos = positions[i-1] if i > 0 else root
   - joint_offset = joint.pos (dx, dy)
   - joint_world = parent_pos + rotate(joint_offset, cumulative_angle)
   - bone_end = joint_world + (length * cos(cumulative_angle), length * sin(cumulative_angle))
   - positions.append(bone_end)
3. 返回所有 joint_world 位置

简化方案：忽略 `pos` offset 的旋转（offset 是局部坐标），直接累加：

```
pos[0] = (root_x + joints[0].pos.dx, root_y + joints[0].pos.dy)
bone_end[0] = pos[0] + (length * cos(angle), length * sin(angle))
pos[1] = bone_end[0] + joints[1].pos offset
...
```

### Step 3：`defskeleton` 特殊形式 `pml/skeleton/evaluator.py` (NEW)

解析 `(defskeleton name (root-x root-y) (joint ...) ...)` 语法：

```python
def _eval_defskeleton(expr: Expr, env: Environment) -> None:
    """(defskeleton <name> (<root-x> <root-y>)
       (joint <joint-name> :pos (<dx> <dy>) :length <len> :angle <angle>
              [:min <min>] [:max <max>]) ...)"""
    # 1. Parse name (Symbol)
    # 2. Parse root params (list of 2 Symbols)
    # 3. Parse each joint clause:
    #    - First element: joint name (Symbol)
    #    - Then keyword-value pairs: :pos, :length, :angle, :min, :max
    # 4. Create SkeletonTemplate, define in env
```

需要在 `pml/evaluator.py` 中注册：`SPECIAL_FORMS["defskeleton"] = _eval_defskeleton`

### Step 4：FABRIK 求解器 `pml/skeleton/ik_fabrik.py` (NEW)

```python
def solve_fabrik(
    skeleton: SkeletonInstance,
    end_effector: str,
    target_x: float,
    target_y: float,
    max_iterations: int = 10,
    tolerance: float = 0.01,
) -> bool:
    """Solve IK using FABRIK algorithm.
    
    Returns True if converged, False otherwise.
    
    Algorithm:
    1. Compute world positions via forward kinematics
    2. Identify the chain from root to end_effector
    3. Backward reach: move end effector to target, adjust chain
    4. Forward reach: fix root, adjust chain back
    5. Convert final positions back to angles
    6. Apply angle constraints (clamp)
    """
```

核心步骤：
- `_positions_to_angles(positions, root)` — 从位置链反算每个关节的角度
- `_backward(positions, target, lengths)` — 从末端向根逐段约束
- `_forward(positions, root, lengths)` — 从根向末端逐段恢复

### Step 5：CCD 求解器 `pml/skeleton/ik_ccd.py` (NEW)

```python
def solve_ccd(
    skeleton: SkeletonInstance,
    end_effector: str,
    target_x: float,
    target_y: float,
    max_iterations: int = 10,
    tolerance: float = 0.01,
) -> bool:
    """Solve IK using Cyclic Coordinate Descent.
    
    Returns True if converged, False otherwise.
    
    Algorithm:
    For each iteration:
      For each joint from end-1 to root:
        1. Compute current end effector position
        2. Compute vector from joint to end effector
        3. Compute vector from joint to target
        4. Rotate joint by the angle between these vectors
        5. Clamp angle if constraints exist
      Check convergence
    """
```

### Step 6：内置函数 `pml/skeleton/__init__.py` (NEW)

注册 5 个 PML 内置函数：

| 函数 | 签名 | 说明 |
|------|------|------|
| `instantiate-skeleton` | `(template (root-x root-y) [:name str])` | 从模板创建实例 |
| `ik-solve` | `(instance end-effector target-x target-y [:method] [:iterations] [:tolerance])` | IK 求解 |
| `bind-skin` | `(graphic instance joint-name ...)` | 绑定图形到关节 |
| `joint-position` | `(instance joint-name)` | 查询关节世界坐标 |
| `defskeleton` | 特殊形式（在 evaluator.py 注册） | 定义骨骼模板 |

### Step 7：皮肤绑定与帧集成

`bind-skin` 在 `SkeletonInstance._bindings` 中记录 `(graphic_id, [joint_names])`。

帧渲染时，`every-frame` 钩子（已在 Timeline 中存在）负责调用 `ik-solve`。皮肤绑定通过以下机制生效：

在 `pml/animation/timeline.py` 的 `render_frames()` 中，frame hooks 已经每帧调用。用户在 PML 代码中通过 `(every-frame (lambda () (ik-solve ...)))` 即可驱动骨骼。

为了让绑定的图形自动跟随关节移动，在 `render_frames()` 中添加**骨骼后处理步骤**：
1. 执行 frame hooks
2. 遍历所有 SkeletonInstance 的 bindings
3. 对每个绑定的 graphic，根据关节位置计算新的 transform
4. 替换 canvas.objects 中对应的对象

需要全局骨骼注册表（模块级 dict）来追踪所有活跃的 skeleton 绑定。

### Step 8：管道集成

**`pml/evaluator.py`** (MODIFY) — 添加 `SPECIAL_FORMS["defskeleton"]` 导入

**`pml/repl.py`** (MODIFY) — 添加 `register_skeleton(env)` 调用

**`pml/api.py`** (MODIFY) — `PMLRuntime` 增加骨骼相关方法（可选）

### Step 9：测试 `tests/test_phase8.py` (NEW)

```python
class TestSkeletonTemplate:     # 5 tests — 创建模板、关节定义、参数验证
class TestSkeletonInstance:      # 6 tests — 实例化、正运动学、关节位置、角度钳位
class TestDefskeleton:           # 4 tests — PML 特殊形式解析、环境绑定
class TestFABRIK:                # 7 tests — 收敛、约束、不可达目标、单关节
class TestCCD:                   # 6 tests — 收敛、约束、多关节链
class TestIKSolve:               # 4 tests — PML ik-solve 内置、method 选择、容差
class TestBindSkin:              # 4 tests — 绑定注册、帧更新、关节跟随
class TestJointPosition:         # 3 tests — 查询、正运动学一致性
class TestPMLIntegration:        # 5 tests — 端到端 PML 代码（骨骼 + IK + GIF）
```

预估：约 44 个测试

---

## 新增/修改文件

```
pml/
├── skeleton/                    (NEW)
│   ├── __init__.py              — register_skeleton()
│   ├── skeleton.py              — Joint, SkeletonTemplate, SkeletonInstance
│   ├── evaluator.py             — _eval_defskeleton 特殊形式
│   ├── ik_fabrik.py             — FABRIK 求解器
│   └── ik_ccd.py                — CCD 求解器
├── errors.py                    (MODIFY) — IKNoSolutionError
├── evaluator.py                 (MODIFY) — SPECIAL_FORMS["defskeleton"]
├── repl.py                      (MODIFY) — register_skeleton
└── animation/
    └── timeline.py              (MODIFY) — 骨骼绑定后处理

tests/
└── test_phase8.py               (NEW) — 骨骼与 IK 测试
```

**预估**：新增约 600-800 行 Python
