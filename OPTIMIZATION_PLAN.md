# PML-CPP 优化计划

> 完整的代码质量、性能、功能和扩展性优化路线图

---

## 一、核心目标

1. **性能优化**：减少内存分配开销，提升解释器执行速度
2. **代码质量**：消除重复代码，改善架构设计
3. **功能完善**：补充缺失的语言特性
4. **扩展性提升**：让语言关键字和特殊形式更容易扩展

---

## 二、扩展性优化（最高优先级）

### 2.1 当前扩展机制分析

PML 当前有三层扩展机制：

| 层级 | 机制 | 扩展方式 | 难度 |
|------|------|----------|------|
| 词法层 | Lexer | 硬编码 TokenType | 高（需改 C++） |
| 语法层 | Parser + Special Forms | `register_special_form()` | 中（需改 C++） |
| 运行时层 | Builtins | `env->define(name, BuiltinProcedure)` | 中（需改 C++） |
| 用户层 | Macros | `(defmacro name (params) body...)` | 低（纯 PML） |

**问题**：添加新的特殊形式（如 `unless`、`when`、`case`）需要修改 C++ 代码并重新编译。

### 2.2 方案：用户可定义的特殊形式（User-Definable Special Forms）

**目标**：让用户可以从 PML 代码中定义新的控制结构，无需修改 C++。

#### 2.2.1 方案 A：基于宏的扩展（推荐）

利用现有的宏系统，提供高层抽象：

```scheme
;; 定义 when 宏（已可用 defmacro 实现）
(defmacro when (test . body)
  `(if ,test (begin ,@body)))

;; 定义 unless 宏
(defmacro unless (test . body)
  `(if (not ,test) (begin ,@body)))

;; 定义 loop 宏（简单的无限循环）
(defmacro loop body
  `(letrec ((forever (lambda () ,@body (forever))))
     (forever)))
```

**优点**：无需任何改动，现有系统已支持
**缺点**：早期宏是非卫生的，可能变量捕获
**现状**：✅ 卫生宏已实现，`defmacro` 会自动重命名宏体内的绑定变量，避免捕获。

#### 2.2.2 卫生宏（Hygienic Macros）

**目标**：实现卫生宏，防止变量捕获。

```scheme
;; 卫生宏示例：swap 不会捕获外部的 tmp
(defmacro swap (a b)
  (let ((tmp (gensym)))  ;; gensym 生成唯一符号
    `(let ((,tmp ,a))
       (set! ,a ,b)
       (set! ,b ,tmp))))
```

**实现方案**：
1. 在 `Macro::expand()` 中自动对宏体中的绑定变量重命名
2. 使用 `gensym` 生成唯一标识符
3. 跟踪宏展开的 lexical scope

**工作量**：中等（约 2-3 天）
**现状**：✅ 已完成，`Macro::expand()` 在展开时对 `lambda`/`let`/`let*`/`define`/`set!` 等绑定变量进行重命名。

#### 2.2.3 方案 C：用户可注册的特殊形式（高级）

**目标**：允许用户从 PML 代码注册新的特殊形式。

```scheme
;; 定义一个编译期转换函数
(define-special-form 'unless
  (lambda (args env)
    (let ((test (car args))
          (body (cdr args)))
      (evaluate `(if (not ,test) (begin ,@body)) env))))
```

**实现方案**：
1. 添加 `define-special-form` 内建函数
2. 在 `evaluator.cpp` 的 dispatch 中检查用户注册的特殊形式
3. 用户注册的 form 接收未求值的参数和环境

**工作量**：较大（约 3-5 天）
**风险**：可能影响性能（每次调用都要查找用户表）

#### 2.2.4 推荐方案

**短期**：方案 A（无需改动）+ 方案 B（卫生宏）
**长期**：方案 C（用户可注册特殊形式）

### 2.3 关键字扩展指南

当前 PML 的"关键字"有两类：

1. **词法关键字**：`#t`、`#f`、`:keyword`（Lexer 硬编码）
2. **语法关键字**：`if`、`define`、`lambda` 等（Special Forms）

**扩展建议**：

| 需求 | 推荐方式 | 示例 |
|------|----------|------|
| 新的控制结构 | 宏 | `when`、`unless`、`case` |
| 新的语法糖 | 宏 + 解析器扩展 | `=>`（match 箭头） |
| 新的参数标记 | `:keyword`（已支持） | `:fill`、`:stroke` |
| 新的字面量类型 | 需改 Lexer | `#r{regex}` |

**最佳实践**：
- 优先用宏实现新语法
- 避免添加新的词法关键字（会破坏向后兼容）
- 使用 `:keyword` 作为命名参数标记

### 2.4 扩展性示例库

以下宏均可直接用 `defmacro` 定义，无需修改 C++：

```scheme
;; case — 多分支匹配（使用 equal?）
(defmacro case (value . clauses)
  (let ((v (gensym)))
    `(let ((,v ,value))
       (cond
         ,@(map (lambda (c)
                  (if (eq? (car c) 'else)
                    `(else ,@(cdr c))
                    `((equal? ,v ',(car c)) ,@(cdr c))))
                clauses)))))

;; -> — 线程宏，把前一个表达式结果插入后一个的第一个参数位置
(defmacro -> (x . forms)
  (if (null? forms)
    x
    (let ((first (car forms))
          (rest (cdr forms)))
      (if (list? first)
        `(-> ,(cons (car first) (cons x (cdr first))) ,@rest)
        `(-> (,first ,x) ,@rest)))))

;; 使用示例
(-> 5
    (+ 3)        ; => (5 + 3) => 8
    (* 2))       ; => (8 * 2) => 16
```

**组织方式**：把这些宏放到 `lib/control.pml`，通过 `(import "lib/control.pml")` 加载即可扩展语言。

---

## 三、内存与性能优化

### 3.1 Arena 分配器（最高优先级）✅ 已完成

**问题**：每次函数调用、let 绑定、列表操作都会分配大量小对象。

**实现**：`src/pml/core/arena.h`
- 块式 bump allocator，默认 64KB/块
- 提供 `Arena::Allocator<T>` STL 兼容分配器
- `current_arena()` thread_local 指针控制当前作用域

**集成点**（当前状态）：
- ✅ `Expr` / `ListExpr` 节点通过 `make_list()` 自动路由到当前 Arena
- ✅ `PMLRuntime::execute()` 在每次执行时激活 Arena，执行结束后 `reset()`
- ✅ 持久对象（`Procedure::body`、`Macro::body`）通过 `clone_expr_to_heap()` 深拷贝到堆，避免 Arena reset 后悬空
- ⏸ `ValueList` / `Environment`：暂不接入 Arena（它们可能跨 `execute()` 生命周期存在，如闭包、全局绑定；强制接入会引入悬空风险，收益/风险比不高）

**验证**：
- 全量测试通过：`pml-tests` 223/223，`pml-builtins-smoke` 134/134
- CLI stress test（`range`/`sum` 5000 项）正常输出

**预期收益**：在单次执行内大量构造/析构 AST/临时列表的场景下，显著减少 `malloc/free` 调用和内存碎片。

### 3.2 Value 类型优化

**问题**：`Value` 是 17 个 alternative 的 `std::variant`，sizeof 很大。

**方案 A：分层 Value**
```cpp
// 小值直接内联，复杂类型用指针
struct Value {
    enum class Tag { Nil, Int, Double, Bool, Ptr };
    Tag tag;
    union {
        int64_t int_val;
        double double_val;
        bool bool_val;
        void* ptr_val;  // 指向复杂类型
    };
};
```

**方案 B：NaN-boxing（激进）**
```cpp
// 将 double/int/bool/nil 编码到 64 位中
// 利用 NaN 的 payload 存储其他类型
using Value = uint64_t;
```

**推荐**：方案 A（平衡性能和可读性）

### 3.3 Environment 查找优化

**问题**：`lookup` 是 O(n) 的链式查找（n = 作用域深度）。

**方案**：词法作用域静态分析
```cpp
// 编译期：将变量名映射为 (depth, index)
struct VarRef {
    int depth;   // 向外几层
    int index;   // 该层的第几个绑定
};

// 运行时：O(1) 查找
Value& Environment::lookup_fast(VarRef ref) {
    auto* env = this;
    for (int i = 0; i < ref.depth; ++i) env = env->parent.get();
    return env->bindings_by_index[ref.index];
}
```

**工作量**：较大（需要添加编译期分析 pass）

### 3.4 GraphicObject params 优化

**问题**：`params` 是 `unordered_map<string, Value>`，字符串哈希慢。

**方案**：枚举 key + 数组存储
```cpp
enum class ParamKey { X, Y, R, W, H, CX, CY, RX, RY, Points, Text, PathData, ... };

struct GraphicObject {
    std::array<std::optional<Value>, 64> fast_params;  // 按枚举索引
    std::unordered_map<std::string, Value> slow_params;  // fallback
};
```

**预期收益**：渲染路径 20-30% 提升

---

## 四、代码质量优化

### 4.1 快速修复（1-2 小时）

| 文件 | 行号 | 问题 | 修复 |
|------|------|------|------|
| evaluator.cpp | 17, 20 | 重复 `#include <sstream>` | 删除一个 |
| evaluator.cpp | 751-752 | 多余大括号 | 删除 `{` |
| transform.h | 12-14 | cairo 相关代码 | 删除 `#ifdef PML_USE_CAIRO` |
| types.h | 55 | `Symbol::id` 类型不一致 | 统一为 `uint64_t` |

### 4.2 提取公共工具（半天）

**kwargs 解析工具**：
```cpp
// src/pml/core/kwargs.h（新文件）
namespace pml::kwargs {
    std::unordered_map<std::string, Value> parse(const std::vector<Value>& args, size_t start);
    std::string get_string(const std::unordered_map<std::string, Value>& kwargs, const std::string& key, const std::string& default_val);
    int get_int(const std::unordered_map<std::string, Value>& kwargs, const std::string& key, int default_val);
}
```

**删除重复的 `value_to_opt_string`**：
- render.cpp 中的版本移到 types.h

### 4.3 AffineTransform 命名空间修复（半天）

**问题**：`AffineTransform` 在全局命名空间，但 types.h 在 `pml::` 中前向声明。

**修复**：
1. 将 `AffineTransform` 移入 `pml::` 命名空间
2. 更新所有引用（objects.h、transform_builtins.cpp 等）
3. 删除 `::AffineTransform` 前缀

### 4.4 全局单例重构（1-2 天）✅ 已完成

**当前单例**：
- `_current_canvas`
- `g_timeline`
- `StyleRegistry::instance()`
- `PaletteManager::instance()`
- `BackendRegistry::instance()`（保留全局，因静态后端注册发生在程序启动时）

**方案**：引入 `PMLContext`
```cpp
// src/pml/api/context.h
class PMLContext {
public:
    std::shared_ptr<Canvas> current_canvas;
    std::shared_ptr<Timeline> timeline;
    std::unique_ptr<StyleRegistry> styles;
    std::unique_ptr<PaletteManager> palettes;

    static PMLContext& current();
    static PMLContext* current_ptr() noexcept;
    static void set_current(PMLContext* ctx) noexcept;
    void reset();
};

class PMLContextScope { /* RAII activate/restore */ };
```

**实现要点**：
- 每个 `PMLRuntime` 实例拥有独立的 `PMLContext ctx_`
- `execute()` 通过 `PMLContextScope` 将 `ctx_` 激活为线程当前上下文
- 原单例访问器委托到 `PMLContext::current()`：
  - `get_current_canvas()` → `PMLContext::current().current_canvas`
  - `get_or_create_timeline()` → `PMLContext::current().timeline`
  - `StyleRegistry::instance()` → `*PMLContext::current().styles`
  - `PaletteManager::instance()` → `*PMLContext::current().palettes`
- 未显式激活上下文时，使用进程级默认上下文，保持向后兼容

**好处**：
- 支持多实例（多个 `PMLRuntime` 互不干扰）
- 方便测试（每个测试用独立 context）
- 为将来并发做准备（线程局部上下文切换）

**验证**：`pml-tests` 223/223 通过，`pml-builtins-smoke` 155/155 通过。

### 4.5 错误位置追踪（1 天）

**问题**：很多错误使用 `SourceLocation{}`（空位置）。

**方案**：
```cpp
// 让 evaluate() 跟踪当前 Expr 的源码位置
Result<Value> evaluate(const Expr& expr, std::shared_ptr<Environment> env, 
                       SourceLocation loc = SourceLocation{}) {
    // 从 expr 中提取位置信息（需要扩展 Expr）
    // 或者在 ListExpr 中添加 location 字段
}
```

---

## 五、错误报告系统完善（高优先级）

> 目标：提供精确、友好、可定位的错误提示，类似 Rust/TypeScript 的错误输出

### 5.1 当前问题诊断

#### 核心缺陷：AST 节点丢失位置信息

- `Expr`（AST 节点）是 `std::variant`，不携带源码位置
- `Lexer` 和 `Parser` 有精确的 `line/column`（在 `Token` 中）
- 但一旦解析完成，位置信息就丢失了
- `Evaluator` 和 `Builtins` 的错误只能用 `SourceLocation{}`（空位置）

**当前错误输出**：
```
Error: PMLTypeError: expected numeric argument        ← 不知道哪一行！
Error: ArityError: expected 2 arguments, got 3        ← 不知道哪个调用！
Error: UnboundVariableError: unbound variable: foo    ← 不知道在哪！
```

**理想错误输出**：
```
Error: script.pml:12:5: PMLTypeError: expected numeric argument, got "hello"

  12 | (circle :x "hello" :y 50)
       ^
  Hint: 'circle' expects a number for :x, but received a string.

  Call stack:
    at draw-circle (script.pml:12)
    at main (script.pml:20)
```

### 5.2 Phase 1：AST 携带位置信息（核心改造，2-3 天）

**修改 `ListExpr` 结构**：
```cpp
struct ListExpr {
    std::vector<Expr> elements;
    SourceLocation location;                      // 列表的起始位置（'(' 的位置）
    std::vector<SourceLocation> element_locations; // 每个元素的位置
};
```

**修改 Parser**：
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
    
    auto list = std::make_shared<ListExpr>();
    list->elements = std::move(items);
    list->location = list_loc;
    list->element_locations = std::move(item_locs);
    
    return Expr{list};
}
```

**辅助函数**：
```cpp
// 从 Expr 中提取位置信息
SourceLocation get_expr_location(const Expr& expr) {
    if (const auto* list = std::get_if<std::shared_ptr<ListExpr>>(&expr)) {
        return (*list)->location;
    }
    return SourceLocation{};
}
```

### 5.3 Phase 2：Evaluator 位置追踪（1-2 天）

**修改 `evaluate()` 签名**：
```cpp
Result<Value> evaluate(
    const Expr& expr,
    std::shared_ptr<Environment> env,
    SourceLocation current_loc = SourceLocation{});
```

**修改 `apply_function()` 签名**：
```cpp
Result<Value> apply_function(
    const Value& func,
    const std::vector<Value>& args,
    const std::unordered_map<std::string, Value>& kwargs,
    std::shared_ptr<Environment> env,
    SourceLocation call_site);  // 调用位置
```

**修改 `BuiltinProcedure` 签名**：
```cpp
// 新签名（带位置）
using BuiltinFn = std::function<Result<Value>(
    const std::vector<Value>&, Environment&, SourceLocation)>;

// 旧签名（deprecated，向后兼容）
using BuiltinFnLegacy = std::function<Result<Value>(
    const std::vector<Value>&, Environment&)>;
```

**向后兼容**：`BuiltinProcedure` 同时支持两种签名，逐步迁移。

**任务**：更新所有 builtin 实现（约 100+ 个），使错误附带精确位置。

### 5.4 Phase 3：错误格式化增强（1 天）

**实现 `SourceManager` 类**：
```cpp
class SourceManager {
    std::unordered_map<std::string, std::vector<std::string>> file_cache_;
public:
    // 获取文件的第 line 行（1-based）
    std::string get_line(const std::string& filename, int line);
};
```

**实现 `format_error_with_source()` 函数**：
- 显示错误类型和位置
- 显示源代码行
- 显示 `^^^` 标记指向出错位置
- 显示修复建议（Hint）

**集成到 CLI 和 REPL**：
- 修改 `main.cpp` 的 `print_error()`
- 修改 `repl.cpp` 的 `format_error()`

### 5.5 Phase 4：错误上下文和调用栈（2-3 天）

**实现 `CallStack` 类**：
```cpp
class CallStack {
    struct Frame {
        std::string function_name;
        SourceLocation call_site;
    };
    std::vector<Frame> frames_;
public:
    void push(const std::string& name, SourceLocation loc);
    void pop();
    std::string format() const;
};

thread_local CallStack g_call_stack;
```

**在 `apply_function()` 中维护调用栈**：
- 调用前 `push`，返回后 `pop`
- 错误输出中包含调用栈

### 5.6 Phase 5：多错误收集和恢复（1-2 天）

**解析器错误恢复**：Panic-mode 恢复，跳过到下一个 `)` 或顶层表达式，收集多个错误一次性报告。

**聚合错误**：
```cpp
struct AggregateError {
    std::vector<PMLException> errors;
    std::string format() const;
};
```

### 5.7 成功标准

1. **所有错误都有精确的文件名:行号:列号**
2. **错误输出包含源代码片段和 `^^^` 标记**
3. **嵌套函数调用显示调用栈**
4. **解析器可以收集多个错误**
5. **向后兼容，现有代码无需修改**

### 5.8 参考资源

- **Rust 错误报告**：https://blog.rust-lang.org/2016/09/29/rust-1.12.html
- **TypeScript 错误报告**：编译器错误格式
- **Elm 错误报告**：https://elm-lang.org/news/compilers-as-fellow-programmers
- **GCC/Clang 错误格式**：`file:line:col: error: message`

---

## 六、功能增强

### 6.1 卫生宏（2-3 天）✅ 已完成

**目标**：防止宏展开时的变量捕获。

**实现**：
1. 在 `Macro::expand()` 中收集宏体中的绑定变量
2. 用 `gensym` 生成唯一名称
3. 替换宏体中的绑定变量

**验证**：`builtins_smoke.cpp` 中 `hygienic-swap` 测试通过，`tmp` 不会被宏内部捕获。

### 6.2 哈希表类型（1 天）✅ 已完成

```scheme
(define h (make-hash))
(hash-set! h "key" "value")
(hash-ref h "key")  ; => "value"
(hash-keys h)       ; => ("key")
```

**实现**：
- 添加 `ValueHashMap` 类型到 Value variant
- 注册 `make-hash`、`hash-ref`、`hash-set!`、`hash-keys`、`hash-values`、`hash-delete!`、`hash?`

**验证**：`builtins_smoke.cpp` 中 Hash Tables 测试全部通过。

### 6.3 向量类型（1 天）✅ 已完成

```scheme
(define v (make-vector 10 0))
(vector-ref v 5)     ; => 0
(vector-set! v 5 42)
(vector-length v)    ; => 10
```

**实现**：
- 添加 `ValueVector` 类型（`std::vector<Value>`）
- 注册 `make-vector`、`vector-ref`、`vector-set!`、`vector-length`、`vector->list`、`list->vector`、`vector?`

**验证**：`builtins_smoke.cpp` 中 Vectors 测试全部通过。

### 6.4 模块自省 API（半天）✅ 已完成

```scheme
(module-available? "lib/math.pml")  ; => #t
(module-list)                        ; => ("math" "graphics")
(module-exports math-mod)            ; => (pi sin cos tan ...)
```

**实现**：
- 在 `ModuleLoader` 上添加 `is_available()` 和 `loaded_modules()`
- 在 `builtins.cpp` 中注册 `module-available?`、`module-list`、`module-exports`

**验证**：`builtins_smoke.cpp` 中 Module Introspection 测试全部通过。

### 6.5 异常处理机制（2 天）

```scheme
(with-exception-handler
  (lambda (err) (println "Caught:" err))
  (lambda () (error "something went wrong")))
```

**实现**：
- 添加 `with-exception-handler` 特殊形式
- 在 evaluator 中维护异常处理器栈

### 6.6 尾调用优化（TCO）（2-3 天）

**目标**：避免递归深了栈溢出。

**方案**：
- 检测尾调用位置（函数调用的最后一步）
- 复用当前 Environment 而非创建新的

---

## 七、实现质量优化

### 7.1 嵌套 quasiquote 修复（半天）

**问题**：`` `(,a ,`b) `` 处理不正确。

**方案**：添加深度计数器
```cpp
Result<Value> expand_quasiquote(const Expr& tmpl, Environment& env, int depth = 0) {
    // 遇到 quasiquote 增加 depth
    // 遇到 unquote 减少 depth
    // 只在 depth == 0 时执行插值
}
```

### 7.2 string->number 错误处理（半天）

**当前**：用 `try/catch` 包裹 `std::stod`/`std::stoll`。

**修复**：用 `std::from_chars`
```cpp
auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
if (ec == std::errc::invalid_argument) {
    return std::unexpected(type_error("..."));
}
```

### 7.3 deep_equal 结构比较（1 天）

**当前**：对 `GraphicObject` 等 shared_ptr 类型只做指针比较。

**修复**：实现结构相等
```cpp
bool deep_equal(const GraphicObject& a, const GraphicObject& b) {
    return a.shape_type == b.shape_type
        && a.fill == b.fill
        && a.stroke == b.stroke
        && deep_equal_params(a.params, b.params)
        && deep_equal_children(a.children, b.children);
}
```

---

## 八、清理工作

### 8.1 移除 Cairo 相关代码（半天）

- 删除 `transform.h` 中的 `#ifdef PML_USE_CAIRO`
- 删除 `CMakeLists.txt` 中的 `PML_BUILD_CAIRO` 选项
- 删除 `third_party/cairo/` 相关 FetchContent
- 删除 `src/pml/backend/cairo/` 目录

### 8.2 统一代码风格（半天）

- 运行 `clang-format` 格式化所有源文件
- 添加 `.clang-format` 配置文件
- 在 CI 中添加格式检查

---

## 九、实施路线图

### Phase 1：快速修复（1 周）

- [x] 修复重复 include、多余大括号
- [x] 提取 kwargs 工具到公共位置
- [x] 修复 AffineTransform 命名空间
- [x] 移除 Cairo 相关代码
- [ ] 统一 Symbol::id 类型

### Phase 2：错误报告系统（2 周）

- [x] AST 携带位置信息（修改 ListExpr、Parser）
- [x] Evaluator 位置追踪（修改 evaluate、apply_function）
- [x] BuiltinProcedure 签名改造（带 SourceLocation）
- [x] 错误格式化增强（SourceManager、源代码片段显示）
- [x] 调用栈追踪（CallStack 类）
- [x] 多错误收集和恢复（Panic-mode 解析）

> 已完成并验证：`pml-builtins-smoke.exe` 134/134 通过；`test_multi_error.pml` 可一次性报告多个语法错误。

### Phase 3：扩展性增强（2 周）

- [x] 实现卫生宏
- [x] 添加哈希表类型
- [x] 添加向量类型
- [x] 实现模块自省 API
- [x] 编写扩展性文档

### Phase 4：性能优化（3 周）

- [x] 实现 Arena 分配器（`src/pml/core/arena.h`，`ListExpr` 接入，`PMLRuntime` 激活）
- [x] 持久对象安全（`clone_expr_to_heap`，`Procedure`/`Macro` 体深拷贝到堆）
- [x] 验证：全量测试通过 + CLI stress test
- [x] 优化 GraphicObject params（枚举 key）— 已完成：`src/pml/graphics/params.h` 定义 `ParamKey` + `Params` 紧凑数组存储；所有图形原语、sprite 组件、渲染后端改用 `ParamKey` 访问
- [x] 优化 Value 类型（分层表示）— 已完成：将 `Value` 从 22 分支 `std::variant` 重构为分层类；小值（nil/int64/double/bool）内联，大值/复杂类型（string/symbol/keyword/list/vector/hashmap/graphic-object/transform/animation 等）通过 `std::shared_ptr<Box>` 堆存；替换所有 `std::visit`/`std::get_if`/`std::holds_alternative` 用法；`value_to_string` 输出格式保持不变
- [x] Environment 词法索引 — 已完成：Environment 新增 VarRef + indexed_values_ + name_to_index_ + varref_cache_，lookup 先解析并缓存词法地址，后续访问 O(1)

### Phase 5：功能完善（2 周）

- [x] 异常处理机制 — 已完成：新增 `with-exception-handler` 特殊形式，thread_local 异常处理器栈，thunk 出错时调用 handler 并以其返回值作为结果
- [x] 尾调用优化 — 已完成：EvalResult/TailCall + trampoline 实现，50000 层递归测试通过
- [x] 嵌套 quasiquote 修复 — 已完成：expand_quasiquote 增加 depth 参数
- [x] string->number 错误处理改进 — 已完成：空/空白字符串、部分解析、溢出均返回精确错误
- [x] deep_equal 结构比较 — 已完成：支持 ValueList / ValueVector / ValueHashMap 递归比较

### Phase 6：代码质量（1 周）

- [x] 全局单例重构（PMLContext）— 已完成：新增 `src/pml/api/context.h/cpp`，`PMLRuntime` 拥有独立 `PMLContext`；canvas / timeline / style / palette 访问委托到线程局部当前上下文；`BackendRegistry` 保留全局单例（静态后端注册）
- [x] 统一代码风格 — 已完成：新增 `.clang-format`（4 空格、100 列、Attach braces、左对齐指针/引用）；已格式化本次 PMLContext 重构涉及的全部源文件；其余文件可在后续单独提交中全局格式化

---

## 十、关键字扩展最佳实践

### 10.1 优先使用宏

```scheme
;; 好的做法：用宏扩展语法
(defmacro when (test . body)
  `(if ,test (begin ,@body)))

;; 不好的做法：添加新的特殊形式
;; (unless test body...)  ; 需要改 C++
```

### 10.2 使用 :keyword 作为参数标记

```scheme
;; 好的做法：用 :keyword
(circle :x 100 :y 100 :r 50 :fill "red")

;; 不好的做法：用位置参数
(circle 100 100 50 "red")  ; 难以阅读和扩展
```

### 10.3 避免添加新的词法关键字

```scheme
;; 好的做法：用符号和宏
(match expr
  ((pattern1) result1)
  ((pattern2) result2))

;; 不好的做法：添加新的词法关键字
;; #match{expr pattern1 => result1}  ; 需要改 Lexer
```

### 10.4 使用模块组织扩展

```scheme
;; lib/control.pml
(provide when unless case match)

(defmacro when ...)
(defmacro unless ...)
(defmacro case ...)
(defmacro match ...)

;; main.pml
(import "lib/control.pml" as control)
(control/when test body...)
```

---

## 十一、成功标准

1. **性能**：解释器速度提升 30%+
2. **扩展性**：用户可以从 PML 代码定义新的控制结构
3. **代码质量**：无重复代码，统一的架构风格
4. **功能完善**：哈希表、向量、异常处理等核心特性可用
5. **向后兼容**：所有现有 PML 程序无需修改即可运行

---

## 附录：参考资源

- **Scheme R5RS**：卫生宏、continuations
- **Racket**：用户可定义的语言扩展
- **Lua**：轻量级嵌入和扩展
- **Julia**：宏系统和元编程

---

*计划创建时间：2026-06-18*
*最后更新：2026-06-18*
