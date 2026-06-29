# PML 六方向语法增强方案

## TL;DR

> **Quick Summary**: 为 PML 实现 6 个语法/可读性增强功能，涵盖匿名函数简写、字面量、线程宏、推导式、声明式场景和隐式 begin。按风险从低到高分阶段交付，零 C++ 改动的功能优先。
>
> **Deliverables**:
> - `#()` 匿名函数 reader macro（Lexer + Parser 改动）
> - `#rgb()`/`#pt()` 颜色/坐标字面量（纯内置函数，零 C++ 改动）
> - `doto` 线程宏（纯 defmacro，零 C++ 改动）
> - `for-comp` 推导式（C++ special form）
> - `scene` 声明式场景（C++ special form）
> - 隐式 begin 回归测试（功能已存在）
>
> **Estimated Effort**: Medium（6 个功能，2-3 个工作日）
> **Parallel Execution**: YES - 3 waves
> **Critical Path**: F6/F3/F2 → F1 → F4/F5 → Final Verification

---

## Context

### Original Request
用户要求实现六方向 PML 语法增强：
1. `#()` 匿名函数简写（类似 Clojure 的 reader macro）
2. 颜色/坐标字面量（`#rgb(r g b)`、`#pt(x y)` 等）
3. `doto` 宏（把值作为首个参数传入，返回原始值）
4. `for` 推导式（`for-comp`，嵌套迭代收集结果）
5. `scene` 声明（声明式 canvas + add + render 包装）
6. 隐式 begin（让 let 等形式的 body 自动支持多表达式）

### 研究与发现

**发现 1：Feature 6 已经完成**
`eval_let_par`（evaluator.cpp:1415-1421）、`eval_let_star`（evaluator.cpp:1466-1473）、`eval_letrec`（evaluator.cpp:1527-1534）的 body 求值循环已经支持多表达式隐式 begin。无需任何代码改动。

**发现 2：Feature 2 可能零 lexer 改动**
`#rgb(r g b)` 在当前的 lexer/parser 中自然解析为列表 `(#rgb r g b)`——`#rgb` 作为 SYMBOL token，`(r g b)` 作为列表。因此 Feature 2 可通过宏或内置函数实现。

**发现 3：Feature 1 需要新 lexer token**
`#(` 当前会被 lexer 读取为 SYMBOL `"#"` + LPAREN，需要新增 FNLIT token type 并在 `read_token()` 中提前拦截。

### Metis Review
**关键建议**：
- **实现顺序**（低风险→高风险）：F6（测试）→ F3（doto 宏）→ F4（for-comp）→ F1（#()）→ F2（字面量）→ F5（scene）
- **锁定 scope**：`#()` 只做匿名函数，不做通用 reader macro；scene 不做响应式 UI；for-comp 不做惰性求值
- **`#rgb` 字面量**的两种路径：零 C++ 改动（宏）或 parser 层 AST 转换
- **Stdlib 嵌入**：每次添加 .pml 文件后，必须重新运行 `tools/embed_stdlib.py`

### User Design Decisions

| Feature | 用户选择 | 理由 |
|---------|---------|------|
| **F2** `#rgb()`/`#pt()` | 方案 A：纯内置函数 | 零 C++ 改动，#rgb(r g b) 自然解析为 (#rgb r g b) |
| **F4** `for-comp` | C++ special form | 规避宏卫生问题，类似现有的 for/dotimes |
| **F5** `scene` | C++ special form | 控制求值顺序，支持运行时 :output 路径 |

---

## Work Objectives

### Core Objective
为 PML 添加 6 个语法糖功能，全面改善可读性和编写体验，零侵入式改动运行时架构。

### Concrete Deliverables

| 功能 | 输出 | 实现方式 | 改动范围 |
|------|------|---------|---------|
| **F1** `#()` | `#(+ %1 1)` → `(lambda (%1) (+ %1 1))` | Reader macro | lexer.h/cpp + parser.h/cpp |
| **F2** `#rgb()`/`#pt()` | `#rgb(255 0 0)` → `"#FF0000"` | 内置函数 | stdlib/color.pml |
| **F3** `doto` | `(doto obj (fn1) (fn2))` | defmacro | stdlib/sugar.pml + 测试 |
| **F4** `for-comp` | `(for-comp (x in xs) (y in ys) body)` | C++ special form | evaluator.cpp + 测试 |
| **F5** `scene` | `(scene w h :output "x.png" els...)` | C++ special form | evaluator.cpp + 测试 |
| **F6** 隐式 begin | `(let ((x 1)) a b c)` | ✅ 已实现 | 仅测试 |

### Definition of Done
- [x] 所有 6 个功能有通过的 GTest 测试（正常路径 + 边界情况）
- [x] 所有 277+233 现有测试全部通过
- [x] 新增及修改的文件完成代码审查
- [x] 生成最终提交

### Must Have
- 所有功能必须是纯语法糖——不改变运行时 Value/Arena/Runtime 系统
- `#()` 不能与 `#t`/`#f` 冲突
- 所有功能必须有 agent-executable QA 场景
- 每次添加 stdlib .pml 后重新运行 `embed_stdlib.py`

### Must NOT Have (Guardrails)
- 不要创建通用 reader macro 系统——`#()` 和 `#xxx()` 是仅有的 `#` 前缀形式
- 不要引入惰性求值序列
- 不要让 `scene` 演变成完整响应式 UI DSL
- Feature 2 不要添加 `#{}` 集合、`#""` 正则等非目标字面量
- Feature 4 不要添加 `:when`/`:while` 守卫过滤（后续可扩展）

---

## Verification Strategy

> **ZERO HUMAN INTERVENTION** — ALL verification is agent-executed.

### Test Decision
- **Infrastructure exists**: YES (GTest)
- **Automated tests**: Tests-after (每个功能写测试）
- **Framework**: GTest (native), `pml::test::eval()` 一键集成测试

### QA Policy
每个 TODO 都包含 agent-executed QA 场景，使用 `pml::test::eval()` 验证：
- **正常路径**: `pml::test::eval("(#(+ %1 1) 5)")` → 预期值
- **边界情况**: 空 body、嵌套、引用冲突
- **回归**: `ctest --preset debug` 全量

---

## Execution Strategy

```
Wave 0 (Zero C++ Risk — 可并行):
├── Task 1: 隐式 begin 回归测试 [quick]
├── Task 2: doto 宏 + 测试 [quick]
└── Task 5: #rgb/#pt 字面量（纯宏，零 C++ 改动）[quick]

Wave 1 (Evaluator Special Forms):
├── Task 3: for-comp C++ special form [unspecified-high]
└── Task 6: scene C++ special form [deep]

Wave 2 (Lexer/Parser):
└── Task 4: #() reader macro — lexer token + parser [deep]

Wave 3 (Build + Finalize) ✓:
└── Task 7: 重新嵌入 stdlib + 全量测试 [quick] (done)

Wave FINAL ✓:
├── F1: Plan compliance audit [oracle] — APPROVE
├── F2: Code quality + full test suite [unspecified-high] — APPROVE
├── F3: QA 场景手动验证 [unspecified-high] — APPROVE
└── F4: Scope fidelity check [deep] — APPROVE
```

---

## TODOs

### Wave 0 — Foundation (零 C++ 风险，可并行)

> 本波任务全部不需要改动 C++ 代码，可以并行执行。
> Stdlib macro 修改后需要重新运行 `tools/embed_stdlib.py` 再构建。

- [x] 1. 隐式 begin 回归测试

  **What to do**:
  - 在 `tests/test_stdlib_macros.cpp` 中添加回归测试
  - 验证 `let`、`let*`、`letrec`、`begin` 的 body 多表达式支持

  **Must NOT do**:
  - 不要改动任何 C++ 代码

  **Recommended Agent Profile**:
  - **Category**: `quick`
  - **Skills**: [] (简单 GTest 添加)

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 0 (with Tasks 2, 5)
  - **Blocks**: Nothing
  - **Blocked By**: None

  **References**:
  - `src/pml/evaluator/evaluator.cpp:1415-1421` — eval_let_par 的 body 循环（已支持多表达式）
  - `src/pml/evaluator/evaluator.cpp:1466-1473` — eval_let_star 的 body 循环（已支持多表达式）
  - `src/pml/evaluator/evaluator.cpp:1527-1534` — eval_letrec 的 body 循环（已支持多表达式）
  - `tests/test_stdlib_macros.cpp` — 测试文件，已有 ThreadFirst/ThreadLast 测试

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY)**:
  ```
  Scenario: let multi-body works
    Tool: Bash (ctest via pml-tests.exe)
    Steps:
      1. Use pml::test::eval("(let ((x 1)) (define y (+ x 1)) (+ x y))") → 3
    Expected Result: 3
    Evidence: .omo/evidence/task-01-let-begin.txt

  Scenario: let* multi-body works
    Tool: Bash
    Steps:
      1. eval("(let* ((x 1) (y (+ x 1))) (println x) (+ x y))") → 3
    Expected Result: 3 (println side effect ignored)
    Evidence: .omo/evidence/task-01-letstar-begin.txt

  Scenario: begin multi-expr works
    Tool: Bash
    Steps:
      1. eval("(begin (define x 1) (define y 2) (+ x y))") → 3
    Expected Result: 3
  ```

  **Commit**: YES (groups with Task 1)
  - Message: `test(let): add regression tests for implicit begin in let/let*`
  - Files: `tests/test_stdlib_macros.cpp`

- [x] 2. doto 宏

  **What to do**:
  - 在 `stdlib/sugar.pml` 中添加 `doto` 宏定义
  - `(doto val (fn1 arg1) (fn2 arg2) ...)` → 将 val 作为第一个参数插入每个 `fn` 调用，返回 val
  - 使用 `gensym` 确保卫生宏
  - 在 `tests/test_stdlib_macros.cpp` 中添加测试

  **Must NOT do**:
  - 不要修改 C++ 代码

  **Recommended Agent Profile**:
  - **Category**: `quick`
  - **Skills**: [] (简单 defmacro)

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 0 (with Tasks 1, 5)
  - **Blocks**: Nothing
  - **Blocked By**: None

  **References**:
  - `stdlib/sugar.pml` — 已有 `defn`、`defconst` 宏，遵循相同风格
  - `evaluator.cpp:eval_let_star` — let 用于 gensym 绑定
  - `tests/test_stdlib_macros.cpp` — 测试文件

  **Acceptance Criteria**:

  **QA Scenarios**:
  ```
  Scenario: doto basic chaining
    Tool: Bash (pml::test::eval)
    Steps:
      1. eval("(doto (list 1 2 3) (push! 4) (push! 5))")
    Expected Result: (1 2 3 4 5)
    Evidence: .omo/evidence/task-02-doto-chaining.txt

  Scenario: doto no forms (just val)
    Tool: Bash
    Steps:
      1. eval("(doto 42)")
    Expected Result: 42
    Evidence: .omo/evidence/task-02-doto-nil.txt

  Scenario: doto preserves value (not last fn result)
    Tool: Bash
    Steps:
      1. eval("(doto 0 (+ 1))")
    Expected Result: 0 (doto returns original val, not (+ 0 1) => 1)
    Evidence: .omo/evidence/task-02-doto-preserve.txt
  ```

  **Commit**: YES
  - Message: `feat(stdlib): add doto macro`
  - Files: `stdlib/sugar.pml`

- [x] 5. `#rgb()` / `#pt()` 字面量（纯宏方案）

  **What to do**:
  - 利用 `#rgb(r g b)` 自然解析为 `(#rgb r g b)` 列表的事实
  - 在 `stdlib/color.pml` 中添加/注册 `#rgb` 作为内置函数
  - `(#rgb 255 0 0)` → `"#FF0000"`（或委托给 `color/rgb`）
  - 同样处理 `#hsl(h s l)`、`#pt(x y)`、`#rect(x y w h)` 等
  - 用户确认：使用方案 A（纯宏/内置函数，零 C++ 改动）

  **Must NOT do**:
  - 不要添加 lexer token
  - 不要改动 parser

  **Recommended Agent Profile**:
  - **Category**: `quick`
  - **Skills**: [] (stdlib 内置函数)

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 0 (with Tasks 1, 2)
  - **Blocks**: Nothing
  - **Blocked By**: None

  **References**:
  - `stdlib/color.pml` — 现有颜色处理代码
  - `src/pml/evaluator/builtins.cpp` — 如何注册内置函数
  - `tools/embed_stdlib.py` — 完成后重新嵌入

  **Acceptance Criteria**:

  **QA Scenarios**:
  ```
  Scenario: #rgb basic usage
    Tool: Bash (pml::test::eval)
    Steps:
      1. eval("(list #rgb(255 0 0))")
    Expected Result: ("#FF0000") 或等效颜色值
    Evidence: .omo/evidence/task-05-rgb-basic.txt

  Scenario: #pt basic usage
    Tool: Bash
    Steps:
      1. eval("(list #pt(10 20))")
    Expected Result: (10 20) 或点对象
    Evidence: .omo/evidence/task-05-pt-basic.txt

  Scenario: #hsl basic usage
    Tool: Bash
    Steps:
      1. eval("(list #hsl(0.5 0.8 0.4))")
    Expected Result: 有效的 HSL 颜色值
    Evidence: .omo/evidence/task-05-hsl-basic.txt
  ```

  **Commit**: YES
  - Message: `feat(stdlib): add #rgb/#hsl/#pt literal builtins`
  - Files: `stdlib/color.pml`

---

### Wave 1 — C++ Special Forms（Evaluator 改动，可并行）

> 本波任务添加两个新的 C++ special form：`for-comp` 和 `scene`。
> 它们注册在 `get_mutable_special_forms()` 表中，遵循现有 pattern。

- [x] 3. for-comp 推导式（C++ special form）

  **What to do**:
  - 用户确认：直接实现为 C++ special form（规避宏卫生问题）
  - 在 `evaluator.cpp` 中添加 `eval_for_comp` 函数
  - 语法: `(for-comp (x in xs) (y in ys) ... body)`
  - 语义：嵌套迭代，收集每个 body 求值结果到一个列表中返回
  - 支持的绑定语法：
    - `(x in list-expr)` — 遍历列表
    - `(x in start end)` — 数字范围（类似现有 for）
    - `(x in start end step)` — 带步长的数字范围
  - 注册到 `get_mutable_special_forms()` 表

  **Must NOT do**:
  - 不要添加 `:when` / `:while` 守卫
  - 不要添加惰性求值
  - 内部实现使用 mini trampoline 或普通 for 循环

  **Design Reference**:
  ```cpp
  // 伪代码 — eval_for_comp 的架构
  Result<EvalResult> eval_for_comp(
      const ArenaExprVector& expr, std::shared_ptr<Environment> env,
      SourceLocation call_site)
  {
      // expr = (for-comp (binding1) (binding2) body)
      // 1. 解析 binding 列表 (expr[1] 到 expr[expr.size()-2])
      // 2. 解析 body (expr[expr.size()-1])
      // 3. 递归嵌套迭代，将结果累加到 ValueList
      // 4. 返回 ValueList
  }
  ```

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
  - **Skills**: [] (C++ special form，参照 eval_for 的实现)

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Task 6)
  - **Blocks**: Nothing
  - **Blocked By**: Wave 0

  **References**:
  - `evaluator.cpp:eval_for (958-1022)` — 现有 for 的实现模式
  - `evaluator.cpp:eval_dotimes (912-956)` — dotimes 的实现模式
  - `evaluator.cpp:register_special_form()` — 注册方式
  - `stdlib/iteration.pml` — 现有迭代函数

  **Acceptance Criteria**:

  **QA Scenarios**:
  ```
  Scenario: for-comp basic nested iteration
    Tool: Bash (pml::test::eval)
    Steps:
      1. eval("(for-comp (x in (list 1 2)) (y in (list 3 4)) (list x y))")
    Expected Result: ((1 3) (1 4) (2 3) (2 4))
    Evidence: .omo/evidence/task-03-forcomp-basic.txt

  Scenario: for-comp single binding over list
    Tool: Bash
    Steps:
      1. eval("(for-comp (x in (list 1 2 3)) (* x 2))")
    Expected Result: (2 4 6)
    Evidence: .omo/evidence/task-03-forcomp-single.txt

  Scenario: for-comp empty list
    Tool: Bash
    Steps:
      1. eval("(for-comp (x in (list)) x)")
    Expected Result: ()
    Evidence: .omo/evidence/task-03-forcomp-empty.txt

  Scenario: for-comp over range
    Tool: Bash
    Steps:
      1. eval("(for-comp (i in 0 5) (* i 2))")
    Expected Result: (0 2 4 6 8)  // 或类似
  ```

  **Commit**: YES (groups with Task 3)
  - Message: `feat(evaluator): add for-comp special form for list comprehension`
  - Files: `src/pml/evaluator/evaluator.cpp`

- [x] 6. scene 声明式场景（C++ special form）

  **What to do**:
  - 用户确认：实现为 C++ special form
  - 语法: `(scene w h :keyword-args... elements...)`
  - 支持的参数：
    - `:output "path.png"` — 输出路径（可选，支持运行时计算的表达式）
    - `:bg "#fff"` — 背景色（可选）
    - `:backend "skia"` — 渲染后端（可选，默认 skia）
    - 剩余参数为可求值的图形表达式
  - 展开逻辑：
    1. 解析 kwarg，分离出 canvas 级参数
    2. 创建 canvas `(canvas w h :bg ...)`
    3. 对每个元素表达式求值并 `add` 到 canvas
    4. 如果指定了 `:output`，调用 `render`
    5. 返回 `nil`（或如果没指定 output，返回 canvas）
  - 在 `evaluator.cpp` 中添加 `eval_scene`
  - 注册到 `get_mutable_special_forms()`

  **Must NOT do**:
  - 不要创建响应式/组件式 UI DSL
  - scene 不管理生命周期，不做状态追踪
  - 不要自动调用 `set-backend!`（假设已在前面调用）

  **Design Reference**:
  ```cpp
  // 伪代码 — eval_scene 的架构
  Result<EvalResult> eval_scene(
      const ArenaExprVector& expr, std::shared_ptr<Environment> env,
      SourceLocation call_site)
  {
      // expr = (scene w h :bg "#fff" :output "x.png" element1 element2 ...)
      // 1. 求值 width/height (expr[1], expr[2])
      // 2. 解析关键字参数
      // 3. 求值剩余元素，每步 add 到 canvas
      // 4. 如果 :output 存在则 render
  }
  ```

  **Recommended Agent Profile**:
  - **Category**: `deep`
  - **Skills**: [] (需要理解 evaluator + canvas/add/render 内置函数)

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Task 3)
  - **Blocks**: Nothing
  - **Blocked By**: Wave 0

  **References**:
  - `evaluator.cpp:2038-2077` — 特殊形式注册表
  - `evaluator.cpp:1538` — eval_begin 的模式（顺序求值，返回最后一个）
  - `canvas_builtins.cpp` — canvas/add/render 内置函数
  - `builtins_threading.cpp` — 特殊形式实现参考模式

  **Acceptance Criteria**:

  **QA Scenarios**:
  ```
  Scenario: scene basic usage
    Tool: Bash (pml::test::eval + check output file)
    Steps:
      1. eval("(scene 400 300 :bg \"#fff\" :output \"/tmp/test_scene.png\" (circle 100 100 50))")
      2. Check /tmp/test_scene.png exists and is valid PNG
    Expected Result: valid PNG output file
    Evidence: .omo/evidence/task-06-scene-basic.txt

  Scenario: scene with multiple elements
    Tool: Bash
    Steps:
      1. eval("(scene 800 600 :output \"/tmp/test_scene2.png\" (rect 0 0 800 600 :fill \"red\") (circle 400 300 100 :fill \"blue\"))")
    Expected Result: output file with red bg + blue circle
    Evidence: .omo/evidence/task-06-scene-multi.txt
  ```

  **Commit**: YES
  - Message: `feat(evaluator): add scene special form for declarative canvas`
  - Files: `src/pml/evaluator/evaluator.cpp`

---

### Wave 2 — Lexer/Parser 改动

> 本波任务修改 lexer 和 parser，添加 `#()` reader macro 支持。

- [x] 4. `#()` reader macro — 匿名函数简写

  **What to do**:

  **Lexer 改动**（`src/pml/frontend/lexer.h` + `lexer.cpp`）:
  1. 在 `TokenType` enum 中添加 `FNLIT` token type
  2. 在 `read_token()` 中 `'` / `` ` `` / `,` 处理后添加：
     ```cpp
     if (ch == '#') {
         if (pos + 1 < source.size() && source[pos + 1] == '(') {
             advance(); advance();
             return Token{TokenType::FNLIT, "#(", cur_line, cur_col};
         }
         // else: fall through to read_atom() for #t/#f
     }
     ```
  3. 在 `is_expr_start()` 中添加 `FNLIT`

  **Parser 改动**（`src/pml/frontend/parser.h` + `parser.cpp`）:
  1. 新增 `parse_fnlit()` 方法：
     - 扫描 `#(...)` 的 body 元素直到 RPAREN
     - 遍历 AST 检测 `%` / `%1` / `%2` / … `%9` 符号
     - 确定最大参数编号（如 `#(+ % %1 %2)` → 2 个参数）
     - 替换 `%` → `%1`
     - 生成 `(lambda (%1 %2 ... %n) body...)`
     - 如果 body 为空，报语法错误
  2. 在 `parse_expr()` 的 switch 中添加 `FNLIT` case
  3. 处理嵌套 `#()`：内层先展开，外层看到的是已展开的 lambda

  **Edge cases**:
  - `#()` → 错误（空函数体无意义）
  - `#(42)` → `(lambda () 42)`（无 `%` 引用，零参数）
  - `#(+ %1 %2)` → `(lambda (%1 %2) (+ %1 %2))`
  - `#(%)` → `(lambda (%1) %1)`
  - `%` 在 `quote` 内部不应被替换
  - `%` 在 `quasiquote` + `unquote` 内部应被替换

  **Must NOT do**:
  - 不要扩展为通用 reader macro 系统
  - 不支持 `%&`（rest 参数）
  - 不支持 `%` 作为复合符号的一部分（如 `foo%`）

  **Recommended Agent Profile**:
  - **Category**: `deep`
  - **Skills**: [] (lexer/parser 双文件改动)

  **Parallelization**:
  - **Can Run In Parallel**: NO (独占 lexer/parser)
  - **Parallel Group**: Wave 2
  - **Blocks**: Nothing
  - **Blocked By**: Wave 0, Wave 1

  **References**:
  - `src/pml/frontend/lexer.cpp:89-110` — 现有 reader macro 处理（`'`, `` ` ``, `,`）
  - `src/pml/frontend/lexer.h:18-32` — TokenType 枚举
  - `src/pml/frontend/parser.cpp:65-125` — parse_expr() switch
  - `src/pml/frontend/parser.cpp:127-170` — parse_list() 模式
  - `src/pml/evaluator/evaluator.cpp:1325-1366` — eval_lambda 验证最终 AST 结构
  - `tests/test_lexer.cpp` — lexer 测试
  - `tests/test_parser.cpp` — parser 测试

  **Acceptance Criteria**:

  **QA Scenarios**:
  ```
  Scenario: #() basic single arg
    Tool: Bash (pml::test::eval)
    Steps:
      1. eval("(#(* % 2) 5)")
    Expected Result: 10
    Evidence: .omo/evidence/task-04-fnlit-basic.txt

  Scenario: #() with named args %1 %2
    Tool: Bash
    Steps:
      1. eval("(#(+ %1 %2) 3 4)")
    Expected Result: 7
    Evidence: .omo/evidence/task-04-fnlit-multi.txt

  Scenario: #() works with map
    Tool: Bash
    Steps:
      1. eval("(map #(* % 2) (list 1 2 3))")
    Expected Result: (2 4 6)

  Scenario: #() no % references — zero-arg lambda
    Tool: Bash
    Steps:
      1. eval("(#(42))")
    Expected Result: 42
    Evidence: .omo/evidence/task-04-fnlit-zero.txt

  Scenario: #() empty body errors
    Tool: Bash
    Steps:
      1. eval("(#())")
    Expected Result: parse error
    Evidence: .omo/evidence/task-04-fnlit-empty.txt

  Regression: #t and #f still work
    Tool: Bash
    Steps:
      1. eval("(if #t 1 2)")
    Expected Result: 1
  ```

  **Commit**: YES
  - Message: `feat(syntax): add #() anonymous function reader macro`
  - Files: `src/pml/frontend/lexer.h`, `src/pml/frontend/lexer.cpp`, `src/pml/frontend/parser.h`, `src/pml/frontend/parser.cpp`

---

### Wave 3 — Build + Finalize

  **What to do**:
  - 运行 `python3 tools/embed_stdlib.py` 重新生成 embedded_stdlib.h
  - 重新构建项目：`cmake --build --preset debug`
  - 运行全量测试：`ctest --preset debug`
  - 如果任何测试失败，排查并修复

  **Must NOT do**:
  - 不要手动修改 `src/pml/core/embedded_stdlib.h`

  **Required Agent Profile**:
  - **Category**: `quick`
  - **Skills**: [] (构建脚本)

  **Parallelization**:
  - **Can Run In Parallel**: YES
  -   **Parallel Group**: Wave 3
  - **Blocks**: Nothing
  - **Blocked By**: Task 2 (doto), Task 5 (#rgb) — stdlib 修改

  **References**:
  - `tools/embed_stdlib.py` — 嵌入脚本
  - `src/pml/module/embedded_stdlib.cpp` — 加载器
  - `CMakePresets.json`

  **Acceptance Criteria**:
- [x] `python3 tools/embed_stdlib.py` 成功执行
- [x] `cmake --build --preset debug` 零错误
- [x] `ctest --preset debug` 全部通过

  **Commit**: YES (groups with Task 7)
  - Message: `chore: regenerate embedded stdlib after syntax enhancements`
  - Files: `src/pml/core/embedded_stdlib.h`

---

## Final Verification Wave

- [x] F1. **Plan Compliance Audit** — `oracle` — **APPROVE**
  Read the plan end-to-end. For each feature: verify implementation exists (read file). For each "Must NOT Have": search codebase for forbidden patterns — reject with file:line if found. Check evidence files exist in `.omo/evidence/`. Compare deliverables against plan.
  Output: `Must Have [N/N] | Must NOT Have [N/N] | Tasks [N/N] | VERDICT: APPROVE/REJECT`

- [x] F2. **Code Quality + Full Test Suite** — `unspecified-high` — **APPROVE**
  Run `cmake --build --preset debug` + `ctest --preset debug`. Review all changed files for: `as any`/`@ts-ignore` (C++: implicit casts, etc), empty catches, commented-out code. Check AI slop: excessive comments, over-abstraction.
  Output: `Build [PASS/FAIL] | Tests [N pass/N fail] | Files [N clean/N issues] | VERDICT`

- [x] F3. **QA Scenario Verification** — `unspecified-high` — **APPROVE**
  Execute EVERY QA scenario from EVERY task — follow exact steps, capture evidence. Test cross-feature integration (`#()` inside `for-comp`, `doto` with `#()`). Test edge cases. Save to `.omo/evidence/final-qa/`.
  Output: `Scenarios [N/N pass] | Integration [N/N] | Edge Cases [N tested] | VERDICT`

- [x] F4. **Scope Fidelity Check** — `deep` — **APPROVE**
  For each task: read "What to do", read actual diff (git log/diff). Verify 1:1 — everything in spec was built (no missing), nothing beyond spec was built (no creep). Check "Must NOT do" compliance. Detect cross-task contamination.
  Output: `Tasks [N/N compliant] | Contamination [CLEAN/N issues] | Unaccounted [CLEAN/N files] | VERDICT`

---

## Commit Strategy

- **1**: `test(let): add regression tests for implicit begin in let/let*` — `tests/test_stdlib_macros.cpp`
- **2**: `feat(stdlib): add doto macro` — `stdlib/sugar.pml`
- **3**: `feat(stdlib): add for-comp comprehension macro` — `stdlib/iteration.pml`
- **4**: `feat(syntax): add #() anonymous function reader macro` — `src/pml/frontend/lexer.h`, `lexer.cpp`, `parser.h`, `parser.cpp`
- **5**: `feat(syntax): add #rgb/#pt literal syntax` — `stdlib/color.pml` or `parser.cpp`
- **6**: `feat(syntax): add scene declarative syntax` — `evaluator.cpp`, `evaluator.h`
- **7**: `chore: regenerate embedded stdlib after syntax enhancements` — `src/pml/core/embedded_stdlib.h`

---

## Success Criteria

### Verification Commands
```bash
# Build
cmake --build --preset debug

# Full test suite
ctest --preset debug

# Individual test binary
./build/debug/tests/Debug/pml-tests.exe

# Stdlib smoke tests
./build/debug/tests/Debug/pml-builtins-smoke.exe
```

### Final Checklist
- [x] 隐式 begin 回归测试通过
- [x] doto 宏测试通过（基本链式、零 forms、返回值保留）
- [x] for-comp 推导式测试通过（嵌套、单绑定、空列表）
- [x] `#()` reader macro 测试通过（基本、多参数、map 配合、空 body 报错）
- [x] `#rgb()`/`#pt()` 字面量测试通过
- [x] scene 测试通过（基本、多元素）
- [x] 全量 277+233 测试通过
- [x] 没有 `#t`/`#f` 回归
- [x] 所有 Must NOT Have 被遵守