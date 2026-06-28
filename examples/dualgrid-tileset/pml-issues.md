# PML 优化建议 — 基于 Dual-Grid 瓦片系统实现的问题整理

## 说明

本文档整理了在实现双网格瓦片系统过程中遇到的所有 PML 语言/工具链问题，按严重程度和类别分组。每个问题包含：现象描述、根因分析、对用户的影响、以及优化建议。

---

## P0 — 阻断性问题（导致程序无法运行）

### 1. `uniform-float` 的语义误导与多 uniform 传递失败

**现象**：连续调用 `uniform-float` 设置 6 个 skia uniform 时，Skia 报错 `skia create_shader_with_uniforms: failed to create shader with uniforms`。

**根因**：
- `uniform-float` 每次调用都创建**一个新 shader object**，且只携带**一个 4 字节 float** 的 uniform 数据。
- 当 SkSL 声明了 6 个 `uniform float`（共 24 字节）时，使用 6 次 `uniform-float` 链式调用传递的仍然是 4 字节数据，Skia uniform 校验失败。
- 解决方案是使用 `make-uniforms` + `apply-uniforms` 将 6 个 float 打包为一个数据块一次性传入。

**用户影响**：
- 开发者在查阅 API 时自然会认为"设置 uniform"应该逐个进行，`uniform-float` 的名称强烈暗示这是"设置一个 uniform float"。
- 没有任何错误提示说明数据大小不匹配，Skia 返回的 `failed to create shader with uniforms` 完全无法指导用户找到 `make-uniforms`。
- 这是一个 API 设计问题：两种传参路径并存但文档缺失，且错误路径没有退化提示。

**优化建议**：
- **方案 A**：重构 `uniform-float` 使其能够累积 uniform 数据，延迟到渲染时一次性提交。这样 `uniform-float` 的语义就是"设置一个 uniform"。
- **方案 B**：如果保持当前设计，`uniform-float` 应重命名为 `make-shader-with-uniform` 或类似名称，明确表示"创建一个携带一个 uniform 的新 shader"。
- **方案 C**：`apply-uniforms` 应当兼容单值输入，即 `(apply-uniforms sksl (make-uniforms v))` 应该等价于 `(uniform-float sksl "u_name" v)`。
- **最低要求**：当 uniform 数据大小与 SkSL 声明不一致时给出可读错误，指明期望 N 字节、得到 M 字节。

### 2. `^)` vs `))` — 着色器字符串的括号匹配错误定位偏移

**现象**：着色器字符串末尾少写一个 `)`，报错 `PMLSyntaxError: Unmatched '(' at line 12`，但实际错误在第 93 行（字符串结束处）。

**根因**：
- 着色器字符串是 `(shader "...")` 形式，内部包含大量 `{` `}` 和 `(` `)`。
- 词法/语法分析器在扫描到字符串字面量中的 `)` 时可能提前认为 `(shader` 表单结束。
- 行号报告依赖于 lexer 的括号计数，字符串字面量内部的括号被错误计入。

**用户影响**：
- 对于嵌入大量代码的字符串（shader、SQL 等），括号匹配错误几乎无法定位。
- 开发者不得不在字符串末尾逐个加减 `)` 来试探正确的匹配数。

**优化建议**：
- Lexer 在扫描字符串字面量时不应增加/减少括号计数。
- 或引入多行字符串语法（如 `(shader #[...]#)`）避免括号转义问题。

---

## P1 — 语言设计问题

### 3. `div` vs `quotient` — 整数除法命名不一致

**现象**：`(div a b)` 报错 `unbound variable: div`，正确名称是 `(quotient a b)`。

**根因**：
- PML 中没有 `div` 内建函数，整数除法使用 `quotient`。
- 这在 Lisp 方言中非常规（多数 Lisp 用 `/` 或 `div`）。

**用户影响**：
- 对新用户极不友好。"除法"的最自然联想就是 `div`。
- 即使知道用 `quotient`，也需要记住 PML 特殊的算术函数命名体系。

**优化建议**：
- 提供 `div` 作为 `quotient` 的别名。
- 或在未绑定变量错误中提供建议：`unbound variable: div, did you mean: quotient?`。

### 4. `to-double` 不存在 — 数值类型隐式转换不透明

**现象**：`(to-double x)` 报错 `unbound variable: to-double`，但文档说 PML 有 `Value::Tag::{Int, Double}` 双 tag。

**根因**：
- PML 的 `Value` 系统同时支持 Int 和 Double tag，算术运算通过 `promote_numeric` 隐式转换。
- 但没有提供显式的类型转换函数（`to-double`, `to-int`, `float->int` 等）。
- 某些 PML 内建函数（如 `uniform-float`）内部通过 `value_to_double` 接受 int，但这不是用户可调用的 API。

**用户影响**：
- 开发者无法显式控制数值类型。
- 当需要确保某个值是 Double 时（例如传入 C++ 端 `double` 参数），只能猜测"反正会用隐式转换"。
- 阅读代码时无法确定一个数字是 Int 还是 Double，可能导致理解困难（实际 Int/Double 的双 tag 设计本身就是之前记录过的 memory：[[value-to-double-type-safety]]）。

**优化建议**：
- 添加 `(->double x)` 和 `(->int x)` 显式转换函数。
- 或统一使用 Double（移除 Int tag，所有数字都是浮点数），简化类型系统。

### 5. `dotimes` 不可用，必须用 `for-each` + `range`

**现象**：`(dotimes (i 10) ...)` 报错 `unbound variable: dotimes`。

**根因**：
- PML 没有 `dotimes` 内建，迭代必须用 `(for-each (lambda (i) ...) (range 0 n))` 模式。
- 这不是 bug，是语言设计选择，但对新用户极不直观。

**用户影响**：
- 简单计数循环需要三层嵌套。
- PML 文档/CLAUD.md 中提到了这个语言特性，但错误信息不会指导用户找到正确写法。

**优化建议**：
- 实现 `dotimes` 作为宏或内建函数。
- 或至少在 `dotimes` 报错时提示：`did you mean: (for-each (lambda (i) ...) (range 0 n))?`。

---

## P2 — 调试与诊断问题

### 6. Skia uniform 绑定失败的错误消息无诊断信息

**现象**：`skia create_shader_with_uniforms: failed to create shader with uniforms` — 仅有这句话，没有：
- 期望的数据大小 vs 实际大小
- 哪个 uniform 名称未匹配
- 着色器编译是否成功（已分开但未关联）

**根因**：Skia 后端在 uniform 绑定失败时只返回简单状态码，没有提取详细错误。

**优化建议**：
- 在 PML 层捕获 Skia uniform 绑定错误，补充诊断信息。
- 打印 SkSL 中声明的所有 uniform 名称和类型。
- 打印实际传入的数据布局（名称 → 值，以及总字节数）。

### 7. 着色器编译成功但渲染结果异常 — 无运行时调试手段

**现象**：SkSL 着色器可以编译，但渲染结果全黑/全白/颜色异常，无法确定是 uniform 值问题还是 shader 逻辑问题。

**根因**：
- PML 没有提供任何形式的 shader 调试输出。
- 无法将中间值渲染到临时图像来验证。
- 没有"用常量值替换 uniform"来隔离问题的快速方式。

**优化建议**：
- 添加 `(shader-preview sksl)` 命令，生成一个包含诊断信息的测试图像。
- 添加 `(shader-uniforms sksl)` 命令，列出着色器的所有 uniform 及当前值。
- 支持 `(shader-validate sksl)` 对 uniform 绑定做预检查。

---

## P3 — 文档与发现性问题

### 8. `make-uniforms` + `apply-uniforms` API 无文档

**现象**：这两个函数完全是通过阅读 `api.cpp` 源码或猜测发现的。没有任何文档或示例说明：
- `make-uniforms` 接受什么参数（数量？类型？）
- `apply-uniforms` 如何与 `shader` 字符串交互
- 为什么需要两步而不是一步

**优化建议**：
- 为这两个函数编写文档和示例。
- 考虑合并为一步：`(apply-shader-with-uniforms sksl bl br tl tr ...)`。

### 9. PML 类型系统的隐式转换规则未文档化

**现象**：`uniform-float` 的第二个参数可以是 int 或 double，通过 `value_to_double` 隐式转换。但 `text` 的坐标参数、`translate` 的偏移参数等各函数的转换规则各不相同，没有统一文档。

**优化建议**：
- 为所有"接受数值"的参数明确文档化可接受的类型。
- 考虑统一使用 `promote_numeric` 策略，减少类型相关的 bug。
- 在 CLAUDE.md 中增加类型系统的快速参考。

---

## 总结优先级

| 优先级 | 问题 | 影响 |
|--------|------|------|
| P0 | `uniform-float` 多 uniform 传递失败 | 阻塞性，无替代方案则不可用 |
| P0 | 字符串括号匹配错误行号偏移 | 阻塞性，极大延长调试时间 |
| P1 | `div` 不存在 | 高，每日碰壁 |
| P1 | `to-double` 不存在 | 中，特定场景阻塞 |
| P1 | `dotimes` 不存在 | 中，语法糖缺失 |
| P2 | Skia 错误无诊断信息 | 高，无法调试 |
| P2 | Shader 无运行时调试手段 | 中，只能靠猜测 |
| P3 | `make-uniforms` 无文档 | 高，API 不可发现 |
| P3 | 类型转换规则不透明 | 低，积累性摩擦 |
