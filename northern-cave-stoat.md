# PML 解释器实现计划 — Phase 0 + Phase 1

## 目标

实现 PML 核心解释器的 Phase 0（词法分析 + 语法分析）和 Phase 1（环境系统 + 求值器 + 内置函数），使得解释器可以执行纯计算类 PML 程序（不涉及图形渲染）。

**最终验证标准**：能成功执行一个包含函数定义、递归、条件分支、列表操作的 `.pml` 程序，输出正确结果。

---

## 实现步骤

### Step 1：项目骨架

使用 `uv` 初始化 Python 项目，创建目录结构：

```
W:\Project\PML\
├── pml/
│   ├── __init__.py
│   ├── lexer.py
│   ├── parser.py
│   ├── ast_nodes.py
│   ├── evaluator.py
│   ├── environment.py
│   ├── errors.py
│   ├── types.py
│   ├── builtins/
│   │   ├── __init__.py
│   │   ├── arithmetic.py
│   │   ├── comparison.py
│   │   ├── logic.py
│   │   ├── list_ops.py
│   │   ├── string_ops.py
│   │   ├── type_predicates.py
│   │   └── io.py
│   └── repl.py
├── tests/
│   ├── test_lexer.py
│   ├── test_parser.py
│   ├── test_evaluator.py
│   └── fixtures/
│       └── fibonacci.pml
├── examples/
│   └── hello.pml
├── pyproject.toml
└── README.md
```

使用 `uv init` + `pyproject.toml` 配置，开发依赖：`pytest`、`ruff`。

### Step 2：AST 节点定义 (`pml/ast_nodes.py` + `pml/types.py`)

- `Symbol` frozen dataclass（`name: str`）
- `Keyword` frozen dataclass（`name: str`，不含冒号前缀）
- `Expr` 类型别名 = `int | float | str | bool | None | list`
- `Procedure` 类（用户定义函数/闭包）
- `BuiltinProcedure` 类（内置函数包装）

### Step 3：异常类型 (`pml/errors.py`)

- `PMLError` 基类（含 `line`、`column`、`filename` 属性）
- `SyntaxError`、`TypeError`、`UnboundVariableError`、`ArityError`

### Step 4：词法分析器 (`pml/lexer.py`)

按文档 5.3 节实现：

- `TokenType` 枚举：`INTEGER`, `FLOAT`, `STRING`, `SYMBOL`, `BOOLEAN`, `KEYWORD`, `LPAREN`, `RPAREN`, `QUOTE`, `QUASIQUOTE`, `UNQUOTE`, `UNQUOTE_SPLICE`, `EOF`
- `Token` dataclass：`type`, `value`, `line`, `column`
- `Lexer` 类：手写扫描器
  - `tokenize()` → `list[Token]`
  - 跳过空白和注释（`;` 到行尾）
  - 字符串支持 `\"` 和 `\\` 转义
  - 数字：先尝试整数，遇 `.` 转浮点
  - 符号：读到空白或括号为止
  - 关键字：以 `:` 开头的符号
  - 语法糖前缀：`'`, `` ` ``, `,`, `,@`
  - 记录行列号用于错误定位

### Step 5：语法分析器 (`pml/parser.py`)

按文档 5.4 节实现：

- `Parser` 类
  - `parse()` → `list[Expr]`（顶层表达式列表）
  - `parse_expr()` → `Expr`（单个表达式）
  - `parse_list()` → `list`（括号内列表）
  - `parse_atom()` → `Atom`（原子值）
  - 语法糖展开：`'x` → `[Symbol("quote"), x]`，同理 quasiquote/unquote
  - 括号不匹配时抛出 `SyntaxError` 并附位置信息

### Step 6：环境系统 (`pml/environment.py`)

按文档 5.5 节实现：

- `Environment` 类：`parent`, `bindings: dict`
  - `lookup(name)` — 沿作用域链搜索
  - `define(name, value)` — 当前环境定义
  - `set(name, value)` — 修改已有绑定
  - `extend(names, values)` — 创建子环境

### Step 7：求值器 (`pml/evaluator.py`)

按文档 5.6 节实现：

- `evaluate(expr, env)` 主函数
  - 自求值：int, float, str, bool, None → 直接返回
  - Symbol → `env.lookup()`
  - List → 特殊形式 or 函数调用
- 特殊形式 dispatch dict `SPECIAL_FORMS`：
  - `quote` — 返回未求值表达式
  - `if` — 条件求值（短路）
  - `cond` — 多分支
  - `define` — 变量/函数定义（含语法糖）
  - `lambda` — 匿名函数，捕获闭包
  - `let` / `let*` — 局部绑定
  - `begin` — 顺序求值
  - `set!` — 修改变量
  - `do` — 迭代
  - `and` / `or` — 短路逻辑
- `evaluate_arguments()` — 分离位置参数和关键字参数
- `apply(func, args, kwargs)` — 函数调用

### Step 8：内置函数 (`pml/builtins/`)

按文档 2.4 节实现，每个文件注册一组函数到环境：

**arithmetic.py** — `+`, `-`, `*`, `/`, `%`, `abs`, `min`, `max`, `floor`, `ceil`, `round`, `sqrt`, `pow`, `sin`, `cos`, `tan`, `atan2`

**comparison.py** — `=`, `<`, `>`, `<=`, `>=`, `eq?`, `equal?`

**list_ops.py** — `cons`, `car`, `cdr`, `list`, `length`, `append`, `reverse`, `map`, `filter`, `reduce`, `nth`, `range`, `null?`, `list?`

**string_ops.py** — `string-append`, `string-length`, `substring`, `string-ref`, `number->string`, `string->number`, `format`

**type_predicates.py** — `number?`, `string?`, `symbol?`, `boolean?`, `procedure?`, `keyword?`

**io.py** — `print`, `println`, `error`, `assert`, `typeof`

**`__init__.py`** — `register_all(env)` 统一注册

### Step 9：REPL + CLI 入口 (`pml/repl.py` + `__main__`)

- REPL 循环：读取 → tokenize → parse → evaluate → 打印结果
- CLI：`python -m pml <file.pml>` 执行文件
- `pml/__main__.py` 作为入口

### Step 10：测试

- `test_lexer.py` — Token 类型、字符串转义、注释、语法糖前缀、行列号
- `test_parser.py` — 嵌套列表、括号匹配、语法糖展开
- `test_evaluator.py` — 算术、函数定义、闭包、递归、let、条件、列表操作
- `fixtures/fibonacci.pml` — 端到端集成测试

---

## 关键文件与职责

| 文件 | 职责 | 预估行数 |
|------|------|---------|
| `pml/types.py` | Symbol, Keyword, Procedure, BuiltinProcedure | ~80 |
| `pml/errors.py` | 异常类型体系 | ~50 |
| `pml/lexer.py` | 词法分析器 | ~200 |
| `pml/parser.py` | 语法分析器 | ~120 |
| `pml/environment.py` | 环境/作用域 | ~60 |
| `pml/evaluator.py` | 求值器 + 特殊形式 | ~250 |
| `pml/builtins/*.py` | 内置函数（6 个文件） | ~400 合计 |
| `pml/repl.py` | REPL + CLI | ~80 |
| `tests/*.py` | 测试（3 个文件） | ~300 合计 |

**总计**：约 1500-1800 行 Python

---

## 验证方案

### 单元测试
```bash
uv run pytest tests/ -v
```

### 集成测试
创建 `examples/hello.pml`，包含：
- 函数定义与递归（fibonacci、factorial）
- let/let* 局部绑定
- 列表操作（map、filter、reduce）
- 字符串格式化
- 条件分支

执行验证：
```bash
uv run python -m pml examples/hello.pml
```

期望输出正确的计算结果。
