# PML 项目路线图

> 当前版本 v0.2 — 渐变系统完整

---

## 已完成

### 渐变填充系统

| 功能 | 描述 |
|------|------|
| 坐标缩放修复 | 归一化 0-1 → 形状包围盒映射 |
| 线性渐变 | `(linear '((pos color)...) [:x1 :y1 :x2 :y2])` |
| 径向渐变 | `(radial '((pos color)...) [:cx :cy :r])` |
| Sweep 渐变 | `(sweep '((pos color)...) [:cx :cy :start-angle :end-angle])` |
| `:tile-mode` kwarg | clamp / repeat / mirror / decal |
| 11 个绘制函数边界传递 | circle, rect, ellipse, polygon, path + rough 变体 |

### 语言基础增强

| 功能 | 描述 |
|------|------|
| `(div a b)` | 整数除法别名 |
| `(->double x)` / `(->int x)` | 显式类型转换 |
| `(dotimes (i n) body...)` | 数值迭代 |
| Lisp 兼容函数 | first/second/third/rest, null/atom/consp/listp, nth, find/position/count, remove/last/butlast, sort/for-each/apply |
| 字符串函数 | string-upcase/downcase/trim |
| 形参语法 | `&optional`/`&key`/`&rest` |

### 着色器工具

| 功能 | 描述 |
|------|------|
| `(shader-uniforms handle)` | uniform 内省 |
| `(shader-validate handle data)` | uniform 大小预检查 |
| `(compose-with-child sksl child)` | GPU 端 shader 组合 |
| `(compose-with-children sksl list ...)` | 多 child shader 组合 |
| `(eval-shader handle x y)` | 着色器采样调试 |
| Uniform 大小校验诊断 | 编译时校验 |
| 错误消息行号增强 | 括号匹配上下文 |

### 架构改进

| 改进 | 描述 |
|------|------|
| `draw_object` 注册表模式 | 替代 if/else 分派链 |
| `Value` 类型安全 | `to_double()` + `is_number()` tag assert |
| `TextureCache` 挂到 `PMLContext` | 替代全局单例 |

---

## 待办

### P0 — 渐进完善

| # | 任务 | 工作量 |
|---|------|--------|
| 1 | 创建渐变示例 .pml 文件 | `examples/14-gradients/` | ~1h |
| 2 | 更新 `.claude/skills/pml-image/SKILL.md` | ~30min |

### P1 — 架构优化

| # | 任务 | 工作量 |
|---|------|--------|
| 3 | 消除重复代码、修复无防护 `double_val()` | ~2h |
| 4 | 清理 debug 输出、裸 new/delete → `unique_ptr` | ~1h |
| 5 | 解除 `pml_core` → Skia 前向声明依赖 | ~2h |
| 6 | 纹理管线拆分：三角化/UV/烘焙/绘制解耦 | ~4h |
| 7 | 全局变量 → `thread_local` | ~2h |
| 8 | 性能：cotangent UV 权重、顶点去重、自适应细分 | ~3h |

### P2 — 纹理/着色器增强

| # | 任务 | 工作量 |
|---|------|--------|
| 9 | `shader-children` — 反射 `uniform shader` 插槽 | ~1h |
| 10 | `image-shader` — PNG 直接转为 GPU shader child | ~2h |
| 11 | `sksl-type-size` — float2/float3/float4 内存布局 | ~1h |
| 12 | `canvas→shader` — canvas 内容转为 shader | ~3h |
| 13 | uniform 动画 — timeline 驱动 shader 参数 | ~3h |

### P3 — 功能扩展

| # | 任务 | 工作量 |
|---|------|--------|
| 14 | 双网格瓦片问题修复 | ~2h |
| 15 | VS Code 预览增强（语法高亮、自动补全） | ~4h |
| 16 | 地形过渡效果 | ~3h |
| 17 | 动画/timeline 增强（缓动曲线、序列帧） | ~4h |
| 18 | 3D 改进（透视投影、光源、阴影） | ~5h |

---

## 版本路线图

```
v0.2  ── 渐变系统完整 ✅
         ├── 线性/径向/Sweep 渐变 ✅
         ├── 坐标缩放修复 ✅
         ├── :tile-mode ✅
         └── 示例 + docs ⏳

v0.3  ── 架构成熟
         ├── 纹理管线拆分
         ├── 代码清理（去重、unique_ptr）
         └── 全局变量 → thread_local

v0.4  ── 着色器工作室
         ├── shader-children / image-shader
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
cmake --build --preset debug
ctest -C Debug --test-dir build/debug
```

## 设计原则

1. **函数式优先**：新增功能优先考虑无副作用的纯函数形式
2. **向后兼容**：不破坏已有 PML 代码的语义
3. **独立可测**：每个 builtin 需要至少一个 smoke test
4. **值类型安全**：`Value` 的 Int/Double 双 tag 读取处必须有 `is_number()` 检查
