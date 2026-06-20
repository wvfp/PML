# PML 错误报告系统完善计划

> 目标：提供精确、友好、可定位的错误提示，类似 Rust/TypeScript 的错误输出

---

## 一、当前问题诊断

### 1.1 核心缺陷：AST 节点丢失位置信息

**现状**：
- `Expr`（AST 节点）是 `std::variant`，不携带源码位置
- `Lexer` 和 `Parser` 有精确的 `line/column`（在 `Token` 中）
- 但一旦解析完成，位置信息就丢失了
- `Evaluator` 和 `Builtins` 的错误只能用 `SourceLocation{}`（空位置）

**后果**：
```
Error: PMLTypeError: expected numeric argument        ← 不知道哪一行！
Error: ArityError: expected 2 arguments, got 3        ← 不知道哪个调用！
Error: UnboundVariableError: unbound variable: foo    ← 不知道在哪！
```

### 1.2 错误消息不够友好

**现状**：
```
Error: script.pml:12:5: PMLTypeError: expected numeric argument
```

**理想**：
```
Error: script.pml:12:5: PMLTypeError: expected numeric argument, got "hello"

  12 | (circle :x "hello" :y 50)
          ^^^^^^^
  Hint: 'circle' expects a number for :x, but received a string.
```

### 1.3 缺少错误上下文

**现状**：
- 不知道当前在执行哪个函数
- 不知道调用栈
- 不知道出错的表达式是什么

**理想**：
```
Error: script.pml:12:5: PMLTypeError: expected numeric argument

  In function 'draw-circle' called from 'main'
  At expression: (circle :x "hello" :y 50)
  
  12 | (circle :x "hello" :y 50)
          ^^^^^^^
  
  Hint: 'circle' expects a number for :x
  Caused by: string "hello" cannot be converted to number
```

---

## 二、解决方案概览

### 2.1 分阶段实施

| 阶段 | 目标 | 工作量 | 收益 |
|------|------|--------|------|
| **Phase 1** | AST 携带位置信息 | 2-3 天 | 根本性解决 |
| **Phase 2** | Evaluator 位置追踪 | 1-2 天 | 所有错误有位置 |
| **Phase 3** | 错误格式化增强 | 1 天 | 友好的错误输出 |
| **Phase 4** | 错误上下文和调用栈 | 2-3 天 | 完整的调试体验 |
| **Phase 5** | 多错误收集和恢复 | 1-2 天 | 一次性报告多个错误 |

---

## 三、Phase 1：AST 携带位置信息（核心改造）

### 3.1 修改 Expr 结构

**当前**：
```cpp
using Expr = std::variant<
    std::nullptr_t, int64_t, double, std::string, bool,
    Symbol, Keyword, std::shared_ptr<ListExpr>
>;

struct ListExpr {
    std::vector<Expr> elements;
};
```

**改造方案 A：包装结构（推荐）**
```cpp
// 每个 Expr 节点附带位置信息
struct LocatedExpr {
    Expr expr;
    SourceLocation location;
};

// 或者更紧凑：直接在 ListExpr 中存储位置
struct ListExpr {
    std::vector<Expr> elements;
    SourceLocation location;  // 列表的起始位置（'(' 的位置）
    std::vector<SourceLocation> element_locations;  // 每个元素的位置
};
```

**改造方案 B：独立位置表（更灵活）**
```cpp
// 用 ID 关联 Expr 和位置
struct ExprId {
    uint64_t id;
};

class ExprLocationMap {
    std::unordered_map<uint64_t, SourceLocation> locations_;
public:
    void set_location(ExprId id, SourceLocation loc);
    SourceLocation get_location(ExprId id) const;
};
```

**推荐**：方案 A（简单直接，性能更好）

### 3.2 修改 Parser

**当前**：
```cpp
auto Parser::parse_list() -> Result<Expr> {
    const Token& open_token = current();
    // ... 解析元素 ...
    return make_list(std::move(items));  // 丢失位置！
}
```

**改造后**：
```cpp
auto Parser::parse_list() -> Result<Expr> {
    const Token& open_token = current();
    SourceLocation list_loc = {open_token.line, open_token.column, filename};
    
    std::vector<Expr> items;
    std::vector<SourceLocation> item_locs;
    
    while (!at_end() && current().type != TokenType::RPAREN) {
        SourceLocation elem_loc = {current().line, current().column, filename};
        auto expr = parse_expr();
        if (!expr) return std::unexpected(expr.error());
        
        items.push_back(std::move(*expr));
        item_locs.push_back(elem_loc);
    }
    
    // 创建带位置的 ListExpr
    auto list = std::make_shared<ListExpr>();
    list->elements = std::move(items);
    list->location = list_loc;
    list->element_locations = std::move(item_locs);
    
    return Expr{list};
}
```

### 3.3 修改原子类型的位置存储

**问题**：`int64_t`、`double`、`std::string` 等原子类型如何存储位置？

**方案**：用包装结构
```cpp
// 所有原子类型都包装在 Located<T> 中
template<typename T>
struct Located {
    T value;
    SourceLocation location;
};

// Expr 变为：
using Expr = std::variant<
    std::nullptr_t,
    Located<int64_t>,
    Located<double>,
    Located<std::string>,
    Located<bool>,
    Located<Symbol>,
    Located<Keyword>,
    std::shared_ptr<ListExpr>  // ListExpr 已包含位置
>;
```

**或者更简单**：只在 `ListExpr` 中存位置，原子类型的位置通过父列表的 `element_locations` 间接获取。

---

## 四、Phase 2：Evaluator 位置追踪

### 4.1 evaluate() 接收位置参数

**当前**：
```cpp
Result<Value> evaluate(const Expr& expr, std::shared_ptr<Environment> env);
```

**改造后**：
```cpp
Result<Value> evaluate(
    const Expr& expr,
    std::shared_ptr<Environment> env,
    SourceLocation current_loc = SourceLocation{});
```

### 4.2 从 Expr 提取位置

```cpp
Result<Value> evaluate(
    const Expr& expr,
    std::shared_ptr<Environment> env,
    SourceLocation current_loc)
{
    // 从 expr 中提取位置（如果是 ListExpr）
    SourceLocation loc = current_loc;
    if (const auto* list = std::get_if<std::shared_ptr<ListExpr>>(&expr)) {
        loc = (*list)->location;
    }
    
    // 所有错误都附带位置
    return std::visit(
        [&](const auto& arg) -> Result<Value> {
            // ... 各种情况 ...
            
            // Symbol lookup 失败时
            if constexpr (std::is_same_v<T, Symbol>) {
                auto val = env->lookup(arg.name);
                if (!val) {
                    return std::unexpected(
                        unbound_error(loc, arg.name));  // 有位置了！
                }
                return *val;
            }
            
            // List (function call) 失败时
            if constexpr (std::is_same_v<T, std::shared_ptr<ListExpr>>) {
                auto func_val = evaluate((*arg)[0], env, loc);
                if (!func_val) return std::unexpected(func_val.error());
                
                // ... 参数求值 ...
                
                auto result = apply_function(*func_val, args, kwargs, env, loc);
                if (!result) {
                    // 错误已经包含位置
                    return std::unexpected(result.error());
                }
                return *result;
            }
        },
        expr
    );
}
```

### 4.3 apply_function 接收位置

```cpp
Result<Value> apply_function(
    const Value& func,
    const std::vector<Value>& args,
    const std::unordered_map<std::string, Value>& kwargs,
    std::shared_ptr<Environment> env,
    SourceLocation call_site)  // 调用位置
{
    // BuiltinProcedure 调用时传递位置
    if constexpr (std::is_same_v<T, std::shared_ptr<BuiltinProcedure>>) {
        return arg->fn(args, *env, call_site);  // 传递位置给 builtin
    }
}
```

### 4.4 BuiltinProcedure 签名改造

**当前**：
```cpp
using BuiltinFn = std::function<Result<Value>(
    const std::vector<Value>&, Environment&)>;
```

**改造后**：
```cpp
using BuiltinFn = std::function<Result<Value>(
    const std::vector<Value>&, Environment&, SourceLocation)>;
```

**所有 builtin 实现都要更新**：
```cpp
def("+", [](const std::vector<Value>& args, Environment&, SourceLocation loc) -> Result<Value> {
    for (const auto& v : args) {
        if (!is_number(v)) {
            return std::unexpected(
                type_error(loc, std::format("expected numeric argument, got {}", 
                                           value_to_string(v))));
        }
    }
    // ...
});
```

---

## 五、Phase 3：错误格式化增强

### 5.1 读取源代码行

```cpp
class SourceManager {
    std::unordered_map<std::string, std::vector<std::string>> file_cache_;
    
public:
    // 获取文件的第 line 行（1-based）
    std::string get_line(const std::string& filename, int line) {
        if (file_cache_.find(filename) == file_cache_.end()) {
            // 读取文件并缓存
            std::ifstream ifs(filename);
            std::string line_str;
            while (std::getline(ifs, line_str)) {
                file_cache_[filename].push_back(line_str);
            }
        }
        
        const auto& lines = file_cache_[filename];
        if (line < 1 || line > static_cast<int>(lines.size())) {
            return "";
        }
        return lines[line - 1];
    }
};
```

### 5.2 格式化错误输出

```cpp
std::string format_error_with_source(
    const PMLException& err,
    SourceManager& source_mgr)
{
    std::string result;
    
    // 1. 错误类型和位置
    if (err.location.has_value()) {
        const auto& loc = *err.location;
        result += std::format("{}:{}:{}: ", 
                             loc.filename.empty() ? "<stdin>" : loc.filename,
                             loc.line, loc.column);
    }
    result += std::format("{}: {}\n", to_string(err.code), err.message);
    
    // 2. 显示源代码行
    if (err.location.has_value() && !err.location->filename.empty()) {
        const auto& loc = *err.location;
        std::string source_line = source_mgr.get_line(loc.filename, loc.line);
        
        if (!source_line.empty()) {
            // 显示行号和源代码
            result += std::format("\n  {} | {}\n", loc.line, source_line);
            
            // 显示 ^^^ 标记
            if (loc.column > 0) {
                result += std::format("  {} | {}{}\n",
                                     std::string(std::to_string(loc.line).size(), ' '),
                                     std::string(loc.column - 1, ' '),
                                     std::string(1, '^'));
            }
        }
    }
    
    // 3. 显示修复建议
    if (err.repair_hint.has_value() && !err.repair_hint->empty()) {
        result += std::format("\n  Hint: {}\n", *err.repair_hint);
    }
    
    return result;
}
```

### 5.3 集成到 CLI

**当前**（[main.cpp](file:///g:/Project/PML/src/pml/cli/main.cpp#L134-L157)）：
```cpp
static void print_error(const pml::PMLException& err) {
    std::cerr << "Error: ";
    if (err.location.has_value()) {
        // ... 打印位置 ...
    }
    std::cerr << pml::to_string(err.code) << ": " << err.message;
    if (err.repair_hint.has_value()) {
        std::cerr << "\n  Hint: " << *err.repair_hint;
    }
    std::cerr << std::endl;
}
```

**改造后**：
```cpp
static void print_error(const pml::PMLException& err) {
    static SourceManager source_mgr;
    std::cerr << format_error_with_source(err, source_mgr);
}
```

**输出示例**：
```
Error: script.pml:12:5: PMLTypeError: expected numeric argument, got "hello"

  12 | (circle :x "hello" :y 50)
       ^
  Hint: 'circle' expects a number for :x, but received a string.
```

---

## 六、Phase 4：错误上下文和调用栈

### 6.1 调用栈追踪

**目标**：显示函数调用链
```
Error: script.pml:12:5: PMLTypeError: expected numeric argument

  Call stack:
    at draw-circle (script.pml:12)
    at main (script.pml:20)
```

**实现**：
```cpp
class CallStack {
    struct Frame {
        std::string function_name;
        SourceLocation call_site;
    };
    
    std::vector<Frame> frames_;
    
public:
    void push(const std::string& name, SourceLocation loc) {
        frames_.push_back({name, loc});
    }
    
    void pop() {
        if (!frames_.empty()) frames_.pop_back();
    }
    
    std::string format() const {
        std::string result = "\n  Call stack:\n";
        for (auto it = frames_.rbegin(); it != frames_.rend(); ++it) {
            result += std::format("    at {} ({}:{}:{})\n",
                                 it->function_name,
                                 it->call_site.filename,
                                 it->call_site.line,
                                 it->call_site.column);
        }
        return result;
    }
};

// 全局或 thread_local
thread_local CallStack g_call_stack;
```

### 6.2 在 apply_function 中维护调用栈

```cpp
Result<Value> apply_function(
    const Value& func,
    const std::vector<Value>& args,
    const std::unordered_map<std::string, Value>& kwargs,
    std::shared_ptr<Environment> env,
    SourceLocation call_site)
{
    // 获取函数名
    std::string func_name = "<anonymous>";
    if (const auto* proc = std::get_if<std::shared_ptr<Procedure>>(&func)) {
        if ((*proc)->name.has_value()) {
            func_name = *(*proc)->name;
        }
    } else if (const auto* builtin = std::get_if<std::shared_ptr<BuiltinProcedure>>(&func)) {
        func_name = (*builtin)->name;
    }
    
    // 压栈
    g_call_stack.push(func_name, call_site);
    
    // 调用函数
    auto result = /* ... 实际调用 ... */;
    
    // 弹栈
    g_call_stack.pop();
    
    return result;
}
```

### 6.3 在错误中包含调用栈

```cpp
std::string format_error_with_source(
    const PMLException& err,
    SourceManager& source_mgr)
{
    std::string result = /* ... 之前的格式化 ... */;
    
    // 添加调用栈
    result += g_call_stack.format();
    
    return result;
}
```

**输出示例**：
```
Error: script.pml:12:5: PMLTypeError: expected numeric argument, got "hello"

  12 | (circle :x "hello" :y 50)
       ^
  
  Call stack:
    at draw-circle (script.pml:12)
    at main (script.pml:20)
  
  Hint: 'circle' expects a number for :x
```

---

## 七、Phase 5：多错误收集和恢复

### 7.1 解析器错误恢复

**当前**：遇到第一个语法错误就停止

**改造**：Panic-mode 恢复
```cpp
class Parser {
    std::vector<PMLException> errors_;
    
public:
    auto parse() -> Result<std::vector<Expr>> {
        std::vector<Expr> exprs;
        
        while (!at_end()) {
            auto expr = parse_expr();
            
            if (!expr) {
                errors_.push_back(expr.error());
                
                // Panic-mode 恢复：跳过到下一个 ) 或顶层
                skip_to_recovery_point();
                continue;
            }
            
            exprs.push_back(std::move(*expr));
        }
        
        if (!errors_.empty()) {
            // 返回所有错误
            return std::unexpected(errors_[0]);  // 或者返回聚合错误
        }
        
        return exprs;
    }
    
private:
    void skip_to_recovery_point() {
        int depth = 0;
        while (!at_end()) {
            if (current().type == TokenType::LPAREN) {
                ++depth;
            } else if (current().type == TokenType::RPAREN) {
                if (depth > 0) --depth;
            } else if (depth == 0 && current().type != TokenType::RPAREN) {
                // 到达顶层表达式
                return;
            }
            advance();
        }
    }
};
```

### 7.2 聚合错误

```cpp
struct AggregateError {
    std::vector<PMLException> errors;
    
    std::string format() const {
        std::string result = std::format("Found {} errors:\n", errors.size());
        for (size_t i = 0; i < errors.size(); ++i) {
            result += std::format("\n[{}{}] {}\n",
                                 i + 1, errors.size(),
                                 format_error_with_source(errors[i]));
        }
        return result;
    }
};
```

---

## 八、实施路线图

### Phase 1：AST 携带位置信息（2-3 天）

**任务**：
1. 修改 `ListExpr` 添加 `location` 和 `element_locations` 字段
2. 修改 `Parser` 在创建 `Expr` 时填充位置
3. 添加辅助函数 `get_expr_location(const Expr&)`
4. 更新所有 `make_list` 调用

**验证**：
- 编写测试：解析 `(circle :x 10)` 后检查 AST 节点的位置
- 确保位置与源代码对应

### Phase 2：Evaluator 位置追踪（1-2 天）

**任务**：
1. 修改 `evaluate()` 签名，添加 `current_loc` 参数
2. 在 `evaluate()` 中从 `Expr` 提取位置
3. 修改 `apply_function()` 签名，添加 `call_site` 参数
4. 修改 `BuiltinProcedure::BuiltinFn` 签名，添加 `SourceLocation` 参数
5. **更新所有 builtin 实现**（约 100+ 个）

**验证**：
- 编写测试：`(+ 1 "hello")` 应该报告精确位置
- 编写测试：`(undefined-func)` 应该报告精确位置

### Phase 3：错误格式化增强（1 天）

**任务**：
1. 实现 `SourceManager` 类
2. 实现 `format_error_with_source()` 函数
3. 修改 CLI 的 `print_error()` 使用新格式化
4. 修改 REPL 的 `format_error()` 使用新格式化

**验证**：
- 手动测试：运行有错误的 PML 程序，检查输出格式
- 截图对比：Rust/TypeScript 的错误输出

### Phase 4：错误上下文和调用栈（2-3 天）

**任务**：
1. 实现 `CallStack` 类
2. 在 `apply_function()` 中维护调用栈
3. 在错误格式化中包含调用栈
4. 添加 `--no-stack-trace` CLI 选项（可选）

**验证**：
- 编写测试：嵌套函数调用出错时，调用栈正确显示

### Phase 5：多错误收集和恢复（1-2 天）

**任务**：
1. 修改 `Parser` 支持错误恢复
2. 实现 `AggregateError` 结构
3. 修改 CLI 支持多错误显示

**验证**：
- 编写测试：有多个语法错误的程序，一次性报告所有错误

---

## 九、向后兼容性

### 9.1 API 兼容性

**问题**：修改 `BuiltinProcedure::BuiltinFn` 签名会破坏现有代码

**方案**：
- 提供旧签名的重载（deprecated）
- 逐步迁移所有 builtin

```cpp
// 旧签名（deprecated）
using BuiltinFnLegacy = std::function<Result<Value>(
    const std::vector<Value>&, Environment&)>;

// 新签名
using BuiltinFn = std::function<Result<Value>(
    const std::vector<Value>&, Environment&, SourceLocation)>;

// BuiltinProcedure 支持两种
class BuiltinProcedure {
    BuiltinFn fn_;
    BuiltinFnLegacy fn_legacy_;
    
public:
    BuiltinProcedure(std::string name, BuiltinFn fn)
        : name(std::move(name)), fn_(std::move(fn)) {}
    
    BuiltinProcedure(std::string name, BuiltinFnLegacy fn)
        : name(std::move(name)), fn_legacy_(std::move(fn)) {}
    
    Result<Value> call(const std::vector<Value>& args, Environment& env, SourceLocation loc) {
        if (fn_) return fn_(args, env, loc);
        return fn_legacy_(args, env);  // 旧签名忽略位置
    }
};
```

### 9.2 测试兼容性

- 所有现有测试应该继续通过
- 新增测试验证位置信息

---

## 十、成功标准

1. **所有错误都有精确的文件名:行号:列号**
2. **错误输出包含源代码片段和 ^^^ 标记**
3. **嵌套函数调用显示调用栈**
4. **解析器可以收集多个错误**
5. **向后兼容，现有代码无需修改**

---

## 十一、参考资源

- **Rust 错误报告**：https://blog.rust-lang.org/2016/09/29/rust-1.12.html
- **TypeScript 错误报告**：https://www.typescriptlang.org/docs/handbook/2/basic-types.html
- **Elm 错误报告**：https://elm-lang.org/news/compilers-as-fellow-programmers
- **GCC/Clang 错误格式**：`file:line:col: error: message`

---

*计划创建时间：2026-06-18*
