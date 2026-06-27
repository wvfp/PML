# PML Lisp 兼容性升级计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 补齐 PML 与标准 Lisp 之间的功能差距，降低 Lisp 程序员（含 LLM）的使用门槛

**Architecture:** 四阶段递进——纯别名层（零风险）→ 核心求值器增强（&optional/&key）→ 序列函数扩展（独立 builtin）→ 字符串工具补齐（独立 builtin）。每阶段独立可测，不阻塞后续阶段。

**Tech Stack:** C++23, PML evaluator (tree-walking interpreter), GTest/smoke-test framework

## Global Constraints

- Phase 1 不可修改任何既有函数的语义，仅注册别名
- Phase 1.5 不可破坏已有 lambda/apply_function 的行为（. rest 语法必须继续工作）
- 每个新增 builtin 必须至少有 1 个 smoke test
- 所有阶段完成后 `ctest --preset debug` 全部通过
- 遵循代码库现有风格：LLVM .clang-format, 4-space indent, 100 cols
- Phase 1.5 的 Procedure 类修改需向后兼容

---
## 文件结构

### 需修改的文件

| 文件 | 阶段 | 改动内容 |
|------|------|----------|
| `src/pml/evaluator/builtins_list.cpp` | P1, P2 | 新增 first/second/third/rest/nth + find/position/count/remove/remove-if/substitute/last/butlast |
| `src/pml/evaluator/builtins_predicates.cpp` | P1 | 新增 null/atom/consp/listp |
| `src/pml/evaluator/builtins_string.cpp` | P3 | 新增 string-upcase/string-downcase/string-trim |
| `src/pml/core/types.h` | P1.5 | Procedure 类增加 ParamInfo 元数据 |
| `src/pml/evaluator/evaluator.cpp` | P1.5 | eval_lambda 解析 &optional/&key/&rest；apply_function 做参数匹配 |
| `tests/builtins_smoke.cpp` | 全部 | 为每个新函数添加 smoke test |

### 数据流

```
内置函数注册：  builtins_list.cpp → env->define("first", ...)
形参解析流：    PML源码 → eval_lambda → ParamInfo → Procedure
参数匹配流：    (foo :x 1) → apply_function → 匹配关键字 → 填充默认值
```

---

## Phase 1: Lisp 别名层

### Task 1.1: 列表访问别名 first/second/third/rest

**Files:**
- Modify: `src/pml/evaluator/builtins_list.cpp`
- Test: `tests/builtins_smoke.cpp`

**Interfaces:**
- Produces: `(first lst)` `(second lst)` `(third lst)` `(rest lst)`

- [ ] **Step 1: 为现有 car/cdr 提取具名函数**

```cpp
// ---- car 的实现 -------------------------------------------------------------
static Result<Value> car_impl(const std::vector<Value>& args, Environment&) {
    if (args.size() != 1) return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
    const auto* lst = args[0].as_list();
    if (!lst || !*lst || (*lst)->elements.empty())
        return std::unexpected(type_error("car: expected non-empty list"));
    return (*lst)->elements[0];
}

// ---- cdr 的实现 -------------------------------------------------------------
static Result<Value> cdr_impl(const std::vector<Value>& args, Environment&) {
    if (args.size() != 1) return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
    const auto* lst = args[0].as_list();
    if (!lst || !*lst || (*lst)->elements.empty())
        return std::unexpected(type_error("cdr: expected non-empty list"));
    std::vector<Value> rest((*lst)->elements.begin() + 1, (*lst)->elements.end());
    return make_list_value(std::move(rest));
}
```

- [ ] **Step 2: 注册别名，将原有 car/cdr 的 def 改用 car_impl/cdr_impl**

```cpp
    def("car", car_impl);
    def("cdr", cdr_impl);
    def("first", car_impl);
    def("rest", cdr_impl);
    def("second", [](const std::vector<Value>& a, Environment&) -> Result<Value> {
        if (a.size() != 1) return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(a.size())));
        const auto* lst = a[0].as_list();
        if (!lst || !*lst || (*lst)->elements.size() < 2)
            return std::unexpected(type_error("second: list too short"));
        return (*lst)->elements[1];
    });
    def("third", [](const std::vector<Value>& a, Environment&) -> Result<Value> {
        if (a.size() != 1) return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(a.size())));
        const auto* lst = a[0].as_list();
        if (!lst || !*lst || (*lst)->elements.size() < 3)
            return std::unexpected(type_error("third: list too short"));
        return (*lst)->elements[2];
    });
```

- [ ] **Step 3: smoke test**

```cpp
    CHECK("first-basic",  "(first '(10 20 30))",  "10");
    CHECK("second-basic", "(second '(10 20 30))", "20");
    CHECK("third-basic",  "(third '(10 20 30))",  "30");
    CHECK("rest-basic",   "(rest '(10 20 30))",   "(20 30)");
```

- [ ] **Step 4: 构建并验证**

```bash
cd G:/Project/PML && cmake --build --preset debug
./build/debug/tests/Debug/pml-builtins-smoke.exe | grep -E "FAIL|first|second|third|rest"
```

- [ ] **Step 5: 提交**

```bash
git add -A && git commit -m "feat(lisp): add first/second/third/rest aliases"
```


### Task 1.2: 类型谓词别名 null/atom/consp/listp

**Files:**
- Modify: `src/pml/evaluator/builtins_predicates.cpp`
- Test: `tests/builtins_smoke.cpp`

- [ ] **Step 1: 注册别名**

```cpp
    def("null", [](const std::vector<Value>& a, Environment&) -> Result<Value> {
        if (a.size() != 1) return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(a.size())));
        return Value(is_nil(a[0]));
    });
    def("atom", [](const std::vector<Value>& a, Environment&) -> Result<Value> {
        if (a.size() != 1) return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(a.size())));
        return Value(!is_pair(a[0]));
    });
    def("consp", [](const std::vector<Value>& a, Environment&) -> Result<Value> {
        if (a.size() != 1) return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(a.size())));
        return Value(is_pair(a[0]));
    });
    def("listp", [](const std::vector<Value>& a, Environment&) -> Result<Value> {
        if (a.size() != 1) return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(a.size())));
        return Value(is_nil(a[0]) || is_pair(a[0]));
    });
```

- [ ] **Step 2: smoke test**

```cpp
    CHECK("null-true",  "(null nil)", "#t");
    CHECK("null-false", "(null 42)",  "#f");
    CHECK("atom-true",  "(atom 42)",  "#t");
    CHECK("atom-false", "(atom '(1 2))", "#f");
    CHECK("consp-true", "(consp '(1 2))", "#t");
    CHECK("consp-false","(consp nil)", "#f");
    CHECK("listp-true", "(listp '(1 2))", "#t");
    CHECK("listp-nil",  "(listp nil)", "#t");
```

- [ ] **Step 3: 构建、测试、提交**

```bash
git commit -m "feat(lisp): add null/atom/consp/listp aliases"
```


### Task 1.3: nth（参数顺序交换）

**Files:**
- Modify: `src/pml/evaluator/builtins_list.cpp`

- [ ] **Step 1: 注册 nth**

```cpp
    def("nth", [](const std::vector<Value>& a, Environment&) -> Result<Value> {
        if (a.size() != 2) return std::unexpected(arity_error(SourceLocation{}, 2, static_cast<int>(a.size())));
        const auto* lst = a[1].as_list();
        if (!lst || !*lst) return std::unexpected(type_error("nth: second argument must be a list"));
        int64_t idx = to_int64(a[0]);
        if (idx < 0 || static_cast<size_t>(idx) >= (*lst)->elements.size())
            return std::unexpected(general_error(std::format("nth: index {} out of range", idx)));
        return (*lst)->elements[static_cast<size_t>(idx)];
    });
```

- [ ] **Step 2: smoke test**

```cpp
    CHECK("nth-basic",  "(nth 0 '(a b c))", "a");
    CHECK("nth-second", "(nth 1 '(a b c))", "b");
```

- [ ] **Step 3: 构建、测试、提交**

---

## Phase 1.5: 形参语法增强

### 设计

当前 `Procedure` 只存 `std::vector<std::string> params`。增加 `std::optional<ParamInfo>` 存储元数据：

```cpp
struct ParamInfo {
    std::vector<std::string> names;
    size_t required_count = 0;
    size_t optional_count = 0;
    std::vector<Value> defaults;
    bool has_keys = false;
    std::unordered_map<std::string, size_t> key_indices;
    bool has_rest = false;
    std::string rest_name;
};
```

`apply_function` 处理流程：必需参数 → 可选参数（填充默认值）→ 关键字参数（匹配 :key val）→ rest 参数。

### Task 1.5.1: ParamInfo 结构体

**Files:**
- Modify: `src/pml/core/types.h`

- [ ] **Step 1: 在 Procedure 类前定义 ParamInfo**

```cpp
/// Parameter metadata for &optional/&key/&rest support.
struct ParamInfo {
    std::vector<std::string> names;
    size_t required_count = 0;
    size_t optional_count = 0;
    std::vector<Value> defaults;
    bool has_keys = false;
    std::unordered_map<std::string, size_t> key_indices;
    bool has_rest = false;
    std::string rest_name;
};
```

- [ ] **Step 2: 给 Procedure 增加成员**

```cpp
class Procedure {
public:
    std::vector<std::string> params;
    Expr body;
    std::optional<ParamInfo> param_info;
    // ...
};
```

- [ ] **Step 3: 构建验证（无行为变化）**

```bash
cmake --build --preset debug
./build/debug/tests/Debug/pml-tests.exe
```

- [ ] **Step 4: 提交**

```bash
git commit -m "feat(core): add ParamInfo struct for optional/key args"
```


### Task 1.5.2: lambda 解析器 + apply_function

**Files:**
- Modify: `src/pml/evaluator/evaluator.cpp`

**实现思路：** eval_lambda 的参数解析循环增加 `&optional`/`&key`/`&rest` 关键字识别。apply_function 中当 `param_info.has_value()` 时走新匹配路径，否则走旧逻辑。

- [ ] **Step 1: 修改 eval_lambda 解析参数**

在 lambda 参数循环中（~986 行），增加关键字判断：

```cpp
    // 在 extract_symbol_name(p) 之后
    if (*name_opt == "&optional") { parsing_mode = 1; optional_start = params.size(); continue; }
    if (*name_opt == "&key") { parsing_mode = 2; info.has_keys = true; optional_start = params.size(); continue; }
    if (*name_opt == "&rest" || *name_opt == "&body") { parsing_mode = 3; continue; }
```

并处理 `(&optional (name default))` 列表形式。

- [ ] **Step 2: 修改 apply_function**

在 Procedure 分支中，检查 `param_info`：

```cpp
    // 在 has_rest 判断之前
    if (arg->param_info.has_value()) {
        const auto& info = *arg->param_info;
        // 1. 绑定必需参数 (0..required_count-1)
        // 2. 绑定可选参数，缺失则填充默认值
        // 3. 绑定关键字参数（从 kwargs 匹配）
        // 4. 绑定 &rest 参数
        auto call_env = build_call_env_with_param_info(arg, args, kwargs, info);
        return TailCall{arg->body, call_env};
    }
    // 原 has_rest 逻辑做 fallback
```

- [ ] **Step 3: 构建验证向后兼容**

```bash
cmake --build --preset debug
./build/debug/tests/Debug/pml-tests.exe        # 314 tests
./build/debug/tests/Debug/pml-builtins-smoke.exe  # 284 tests
```

- [ ] **Step 4: &optional/&key 的 smoke test**

```cpp
    CHECK("opt-basic",     "((lambda (x &optional y) (list x y)) 1)",       "(1 nil)");
    CHECK("opt-default",   "((lambda (x &optional (y 10)) (list x y)) 1)",  "(1 10)");
    CHECK("opt-supplied",  "((lambda (x &optional y) (list x y)) 1 2)",     "(1 2)");
    CHECK("key-basic",     "((lambda (&key x y) (list x y)) :x 10 :y 20)",  "(10 20)");
    CHECK("key-default",   "((lambda (&key (x 0)) (list x)) :y 1)",         "(0)");
```

- [ ] **Step 5: 提交**

```bash
git commit -m "feat(evaluator): add &optional/&key/&rest parameter syntax"
```


## Phase 2: 序列函数

### Task 2.1: find/position/count

**Files:**
- Modify: `src/pml/evaluator/builtins_list.cpp`

- [ ] **Step 1: 实现 find**

```cpp
    def("find", [](const std::vector<Value>& a, Environment&) -> Result<Value> {
        if (a.size() < 2) return std::unexpected(arity_error(SourceLocation{}, 2, static_cast<int>(a.size())));
        const auto* lst = a[1].as_list();
        if (!lst || !*lst) return std::unexpected(type_error("find: second argument must be a list"));
        for (const auto& e : (*lst)->elements) if (deep_equal(a[0], e)) return e;
        return Value(nullptr);
    });
```

- [ ] **Step 2: 实现 position**

```cpp
    def("position", [](const std::vector<Value>& a, Environment&) -> Result<Value> {
        if (a.size() < 2) return std::unexpected(arity_error(SourceLocation{}, 2, static_cast<int>(a.size())));
        const auto* lst = a[1].as_list();
        if (!lst || !*lst) return std::unexpected(type_error("position: second argument must be a list"));
        for (size_t i = 0; i < (*lst)->elements.size(); ++i)
            if (deep_equal(a[0], (*lst)->elements[i])) return Value(static_cast<int64_t>(i));
        return Value(nullptr);
    });
```

- [ ] **Step 3: 实现 count**

```cpp
    def("count", [](const std::vector<Value>& a, Environment&) -> Result<Value> {
        if (a.size() < 2) return std::unexpected(arity_error(SourceLocation{}, 2, static_cast<int>(a.size())));
        const auto* lst = a[1].as_list();
        if (!lst || !*lst) return std::unexpected(type_error("count: second argument must be a list"));
        int64_t n = 0;
        for (const auto& e : (*lst)->elements) if (deep_equal(a[0], e)) ++n;
        return Value(n);
    });
```

- [ ] **Step 4: smoke test**

```cpp
    CHECK("find-basic",    "(find 30 '(10 20 30 40))",     "30");
    CHECK("find-missing",  "(find 99 '(10 20))",           "nil");
    CHECK("position-basic","(position 30 '(10 20 30 40))", "2");
    CHECK("count-basic",   "(count 30 '(10 20 30 30 40))", "2");
```

- [ ] **Step 5: 提交**

```bash
git commit -m "feat(lisp): add find/position/count sequence functions"
```


### Task 2.2: remove/substitute/last/butlast

**Files:**
- Modify: `src/pml/evaluator/builtins_list.cpp`

- [ ] **Step 1-4: 实现函数并注册**

```cpp
    def("remove", [](const std::vector<Value>& a, Environment&) -> Result<Value> {
        if (a.size() < 2) return std::unexpected(arity_error(SourceLocation{}, 2, static_cast<int>(a.size())));
        const auto* lst = a[1].as_list();
        if (!lst || !*lst) return std::unexpected(type_error("remove: second argument must be a list"));
        std::vector<Value> r;
        for (const auto& e : (*lst)->elements) if (!deep_equal(a[0], e)) r.push_back(e);
        return make_list_value(std::move(r));
    });
    def("last", [](const std::vector<Value>& a, Environment&) -> Result<Value> {
        if (a.size() != 1) return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(a.size())));
        const auto* lst = a[0].as_list();
        if (!lst || !*lst || (*lst)->elements.empty())
            return std::unexpected(type_error("last: expected non-empty list"));
        return (*lst)->elements.back();
    });
    def("butlast", [](const std::vector<Value>& a, Environment&) -> Result<Value> {
        if (a.size() != 1) return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(a.size())));
        const auto* lst = a[0].as_list();
        if (!lst || !*lst || (*lst)->elements.empty())
            return std::unexpected(type_error("butlast: expected non-empty list"));
        std::vector<Value> r((*lst)->elements.begin(), (*lst)->elements.end() - 1);
        return make_list_value(std::move(r));
    });
```

- [ ] **Step 5: smoke test**

```cpp
    CHECK("remove-basic", "(remove 30 '(10 20 30 40))", "(10 20 40)");
    CHECK("last-basic",   "(last '(10 20 30))", "30");
    CHECK("butlast-basic","(butlast '(10 20 30))", "(10 20)");
```

- [ ] **Step 6: 提交**

```bash
git commit -m "feat(lisp): add remove/last/butlist sequence functions"
```


## Phase 3: 字符串增强

### Task 3.1: string-upcase/downcase/trim

**Files:**
- Modify: `src/pml/evaluator/builtins_string.cpp`

- [ ] **Step 1: 实现三个函数**

```cpp
    def("string-upcase", [](const std::vector<Value>& a, Environment&) -> Result<Value> {
        if (a.size() != 1) return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(a.size())));
        const auto* s = a[0].as_string();
        if (!s) return std::unexpected(type_error("string-upcase: argument must be a string"));
        std::string r = *s;
        for (auto& c : r) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return Value(std::move(r));
    });
    def("string-downcase", [](const std::vector<Value>& a, Environment&) -> Result<Value> {
        if (a.size() != 1) return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(a.size())));
        const auto* s = a[0].as_string();
        if (!s) return std::unexpected(type_error("string-downcase: argument must be a string"));
        std::string r = *s;
        for (auto& c : r) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return Value(std::move(r));
    });
    def("string-trim", [](const std::vector<Value>& a, Environment&) -> Result<Value> {
        if (a.size() != 1) return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(a.size())));
        const auto* s = a[0].as_string();
        if (!s) return std::unexpected(type_error("string-trim: argument must be a string"));
        auto start = (*s).find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return Value(std::string{});
        auto end = (*s).find_last_not_of(" \t\n\r");
        return Value((*s).substr(start, end - start + 1));
    });
```

- [ ] **Step 2: smoke test**

```cpp
    CHECK("string-upcase",   "(string-upcase \"hello\")",   "\"HELLO\"");
    CHECK("string-downcase", "(string-downcase \"HELLO\")", "\"hello\"");
    CHECK("string-trim",     "(string-trim \"  hi  \")",    "\"hi\"");
```

- [ ] **Step 3: 提交**

```bash
git commit -m "feat(string): add string-upcase/downcase/trim"
```


## 工作量估算

| Task | 文件 | 代码行 | 时间 |
|------|------|--------|------|
| P1-1 first/second/third/rest | builtins_list.cpp | ~30 | 15min |
| P1-2 null/atom/consp/listp | builtins_predicates.cpp | ~25 | 10min |
| P1-3 nth | builtins_list.cpp | ~15 | 10min |
| P1.5-1 ParamInfo struct | types.h | ~25 | 15min |
| P1.5-2 lambda + apply 改造 | evaluator.cpp | ~120 | 60min |
| P2-1 find/position/count | builtins_list.cpp | ~45 | 20min |
| P2-2 remove/last/butlast | builtins_list.cpp | ~40 | 15min |
| P3-1 string case/trim | builtins_string.cpp | ~35 | 15min |
| smoke tests | builtins_smoke.cpp | ~50 | 15min |
| **合计** | **8 文件** | **~385** | **~2.5h** |

## 验证方式

1. 每 task 后：`cmake --build --preset debug && ./build/debug/tests/Debug/pml-builtins-smoke.exe | grep FAIL`
2. 每阶段后：`ctest --preset debug`（314 + 284 + 9 + 12 全部通过）
3. Phase 1.5 后验证：现有示例 `examples/13-texture/*.pml` 仍正常渲染

## 执行选择

**Plan complete.** 推荐子代理驱动（Subagent-Driven）执行——每个 Task 派发独立的子代理，带审阅检查点。
