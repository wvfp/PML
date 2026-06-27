# PML 项目综合计划

> 整合自多次讨论、代码审阅和功能规划。包含已完成、进行中和待办的所有工作。

---

## 已完成的工作

以下所有阶段均已完成并合并到 `dev-cpp` 分支。

### Phase A — 语言基础增强

| 功能 | 描述 | 文件 |
|------|------|------|
| `(div a b)` | 整数除法别名 | `builtins_arithmetic.cpp` |
| `(->double x)` | 显式类型转换 | `builtins_arithmetic.cpp` |
| `(->int x)` | 显式类型转换 | `builtins_arithmetic.cpp` |
| `(dotimes (i n) body...)` | 数值迭代 special form | `evaluator.cpp` |
| `(shader-uniforms handle)` | 着色器 uniform 内省 | `shader_builtins.cpp`, `backend.h` |
| `(shader-validate handle data)` | uniform 大小预检查 | `shader_builtins.cpp` |
| `(compose-with-child sksl child)` | GPU 端 shader 组合 | `shader_builtins.cpp` |
| `(compose-with-children sksl list ...)` | 多 child shader 组合 | `shader_builtins.cpp` |
| `(eval-shader handle x y)` | 着色器采样调试 | `shader_builtins.cpp` |
| Uniform 大小校验诊断 | 编译时校验 | `skia_backend.cpp` |
| 错误消息行号增强 | 括号匹配上下文 | `parser.cpp` |

### Phase B — 渐变填充系统

| 功能 | 描述 | 文件 |
|------|------|------|
| 坐标缩放修复 | 归一化 0-1 → 形状包围盒映射 | `skia_backend_internal.h`, `skia_backend_draw.cpp` |
| 线性渐变 | `(linear '((pos color)...) [:x1 :y1 :x2 :y2])` | `gradient_builtins.cpp` |
| 径向渐变 | `(radial '((pos color)...) [:cx :cy :r])` | `gradient_builtins.cpp` |
| Sweep 渐变 | `(sweep '((pos color)...) [:cx :cy :start-angle :end-angle])` | `gradient_builtins.cpp` |
| 11 个绘制函数边界传递 | circle, rect, ellipse, polygon, path + rough 变体 | `skia_backend_draw.cpp` |

### Lisp 兼容性（已实现部分）

| 功能 | 描述 |
|------|------|
| `first`/`second`/`third`/`rest` | 列表访问别名 |
| `null`/`atom`/`consp`/`listp` | 类型谓词别名 |
| `nth` | 索引访问（Lisp 风格参数顺序） |
| `&optional`/`&key`/`&rest` | 形参语法增强 |
| `find`/`position`/`count` | 序列搜索函数 |
| `remove`/`last`/`butlast` | 序列操作函数 |
| `string-upcase`/`downcase`/`trim` | 字符串函数 |
| `sort`/`for-each`/`apply` | 高阶函数 |

---

## 待办工作

按优先级从高到低排列。

### P0 — 渐变系统完善

| # | 任务 | 涉及文件 | 工作量 |
|---|------|----------|--------|
| 1 | 添加 `:tile-mode` kwarg（clamp/repeat/mirror/decal） | `gradient_builtins.cpp`, `skia_backend_internal.h`, `gradient.h` | ~1h |
| 2 | 创建渐变示例 .pml 文件 | `examples/14-gradients/`（新建） | ~1h |
| 3 | 更新 `.claude/skills/pml-image/SKILL.md` | `skills/` | ~30min |

### P1 — 架构优化

来自 45 项代码审阅发现的架构改进。

| # | 任务 | 涉及文件 | 工作量 |
|---|------|----------|--------|
| 4 | 消除重复代码、修复无防护 `double_val()` | 多处 | ~2h |
| 5 | 清理 debug 输出、裸 new/delete → `unique_ptr` | 多处 | ~1h |
| 6 | 解除 `pml_core` → Skia 循环依赖 | `core/texture_cache.h` | ~3h |
| 7 | `draw_object` 注册表模式替代 if/else 链 | `skia_backend_draw.cpp` | ~2h |
| 8 | `Value` 类型安全：`to_double()` + tag assert | `core/types.h`, `core/value.h` | ~1h |
| 9 | 纹理管线拆分：三角化/UV/烘焙/绘制解耦 | 纹理管线 | ~4h |
| 10 | `TextureCache` 挂到 `PMLContext` | `asset/`, `core/` | ~1h |
| 11 | 全局变量 → `thread_local` | 各处 | ~2h |
| 12 | 性能：cotangent UV 权重、顶点去重、自适应细分 | 纹理管线 | ~3h |

### P2 — 纹理/着色器增强

| # | 任务 | 涉及文件 | 工作量 |
|---|------|----------|--------|
| 13 | `shader-children` — 反射 `uniform shader` 插槽 | `shader_builtins.cpp`, `backend.h` | ~1h |
| 14 | `image-shader` — PNG 直接转为 GPU shader child | `skia_backend.cpp` | ~2h |
| 15 | `sksl-type-size` — float2/float3/float4 内存布局辅助 | `shader_builtins.cpp` | ~1h |
| 16 | `canvas→shader` — 当前 canvas 内容转为 shader | 后端层 | ~3h |
| 17 | uniform 动画 — timeline 驱动 shader 参数 | `animation/`, `shader_builtins.cpp` | ~3h |

### P3 — 功能扩展

| # | 任务 | 涉及文件 | 工作量 |
|---|------|----------|--------|
| 18 | 双网格瓦片问题修复 | 瓦片系统 | ~2h |
| 19 | VS Code 预览增强（语法高亮、自动补全） | `pml-vscode/` | ~4h |
| 20 | 地形过渡效果 | 瓦片系统 + shader | ~3h |
| 21 | 动画/timeline 增强（缓动曲线、序列帧） | `animation/` | ~4h |
| 22 | 3D 改进（透视投影、光源、阴影） | `graphics3d/` | ~5h |

---

## 技术债务备注

| 项目 | 描述 | 建议 |
|------|------|------|
| `dotimes` 实现 | PML 宏卫生 bug 导致无法用 defmacro 实现，改为 C++ special form | 后续修复宏卫生后可重写为宏 |
| Skia `kHalf` 类型 | 当前 Skia 版本不存在 `kHalf*` 类型枚举 | 升级 Skia 后可恢复统一 case 处理 |
| `pml_core` 依赖 Skia | `texture_cache.h` 引用了 `SkImage` | 移到后端层 |
| `draw_object` 分派 | 巨大的 if/else 链 | 注册表模式 |

---

## 版本路线图

```
v0.2  ── 渐变系统完整（当前）
         ├── 线性/径向/Sweep 渐变 ✅
         ├── 坐标缩放修复 ✅
         └── :tile-mode + 示例 ⏳

v0.3  ── 架构成熟
         ├── 注册表模式 draw_object
         ├── 纹理管线拆分
         ├── Value 类型安全
         └── 循环依赖解除

v0.4  ── 着色器工作室
         ├── shader-children/image-shader
         ├── canvas→shader
         ├── uniform 动画
         └── sksl-type-size

v0.5+ ── 高级功能
         ├── 双网格瓦片
         ├── VS Code 增强
         ├── 3D 增强
         └── 动画系统
```

---

## 构建与验证

```bash
# 构建
cmake --build --preset debug

# 运行全部测试
ctest -C Debug --test-dir build/debug

# 运行 smoke test
./build/debug/tests/Debug/pml-builtins-smoke.exe

# 运行单元测试
./build/debug/tests/Debug/pml-tests.exe --gtest_filter="PMLRuntimeTest.*"
```

---

## 设计原则

1. **函数式优先**：新增功能优先考虑无副作用的纯函数形式
2. **向后兼容**：不破坏已有 PML 代码的语义
3. **独立可测**：每个 builtin 需要至少一个 smoke test
4. **架构分层**：`pml_core` 不依赖后端，`pml_graphics` 不依赖 evaluator
5. **值类型安全**：`Value` 的 Int/Double 双 tag 读取处必须有 `is_number()` 检查或明确的 `to_double()` 调用
