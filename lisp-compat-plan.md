# PML Lisp 兼容性升级计划

## 目标

降低 Lisp 程序员（含 LLM）使用 PML 的门槛，补齐高频缺失函数。

## 实施策略

- **纯别名**（`first` → `car`）：在 builtins 注册时多注册一个名字，共享同一个 lambda
- **转发函数**（`nth` → `list-ref` 参数互换）：新 builtin，内部调用既有实现
- **新函数**（`remove`、`count` 等）：新 builtin，纯新实现

三个策略互不依赖，可并行。

---

## Phase 1: Lisp 别名层（高性价比，3-5 行/个）

### 1.1 列表访问别名

| 别名 | 指向 | 说明 |
|------|------|------|
| `first` | `car` | `(first '(a b c))` → `a` |
| `second` | `cadr` | `(second '(a b c))` → `b` |
| `third` | `caddr` | `(third '(a b c))` → `c` |
| `rest` | `cdr` | `(rest '(a b c))` → `(b c)` |

**实现**：在 `builtins_list.cpp` 的 `register_list_builtins()` 中多注册一次：

```cpp
def("first",  [](auto& a, auto& e) { return builtin_car_impl(a, e); });
def("second", [](auto& a, auto& e) { return builtin_cadr_impl(a, e); });
```

### 1.2 类型谓词别名

| 别名 | 指向 | 说明 |
|------|------|------|
| `null` | `nil?` | `(null x)` 判断空/假 |
| `atom` | 新实现 | `(atom x)` → `(not (pair? x))` |
| `consp` | `pair?` | `(consp x)` 判断是否为序对 |
| `listp` | 新实现 | `(listp x)` → `(or (nil? x) (pair? x))` |

**实现**：在 `builtins_predicates.cpp` 中注册。

### 1.3 参数兼容

| 函数 | 行为 |
|------|------|
| `nth` | `(nth n lst)` → 等价于 `(list-ref lst n)`，参数顺序交换 |

**实现**：新 builtin，参数互换后调用 `list-ref` 的内部逻辑。

---

## Phase 1.5: 形参语法增强（中等偏高，80-120 行）

在 lambda/defn 中添加 Lisp 标准形参关键字支持。

### 支持的语法

```pml
;; &rest — 剩余参数（等价于现有 . 语法）
(defn foo (a b &rest rest) ...)

;; &optional — 可选参数，可带默认值
(defn foo (a &optional (b 10)) ...)
(defn foo (a &optional b) ...)            ;; 默认 nil

;; &key — 关键字参数（调用时自动匹配 :key val）
(defn foo (a b &key x y) ...)
(foo 1 2 :x 10 :y 20)

;; &key + 默认值
(defn foo (&key (x 0) (y 0)) ...)
(foo :x 5 :y 3)
```

### 需要修改的地方

| 文件 | 改动 | 行数 |
|------|------|------|
| `core/types.h` | `Procedure` 类增加 `optional_info`、`key_info` 元数据 | ~15 行 |
| `evaluator.cpp` — `eval_lambda` | 解析 `&optional`/`&key`/`&rest` 关键字 | ~30 行 |
| `evaluator.cpp` — `apply_function` | 根据元数据做参数匹配 + 默认值填充 | ~50 行 |
| `sugar.pml` | 更新 `defn` 宏（如需要） | ~5 行 |

### 设计

```cpp
struct ParamInfo {
    std::vector<std::string> names;          // 所有参数名
    std::vector<bool> optional_mask;         // true = 该参数有默认值
    std::vector<Value> default_values;       // 默认值
    std::vector<std::string> key_to_param;   // keyword → 参数名映射
    size_t min_args;                         // 最少必需参数数
    bool has_rest = false;                   // 有 &rest 参数
    std::string rest_name;                   // &rest 参数名
};
```

### 2.1 查找类

| 函数 | 签名 | 说明 |
|------|------|------|
| `find` | `(find item lst [:test fn])` | 返回第一个匹配元素 |
| `position` | `(position item lst [:test fn])` | 返回第一个匹配位置 |
| `count` | `(count item lst [:test fn])` | 统计匹配次数 |

**实现**：在 `builtins_list.cpp` 中新增，支持可选的 `:test` 关键字（默认 `equal?`）。

### 2.2 删除/替换类

| 函数 | 签名 | 说明 |
|------|------|------|
| `remove` | `(remove item lst [:test fn])` | 删除匹配元素（新列表） |
| `remove-if` | `(remove-if pred lst)` | 删除满足条件的元素 |
| `substitute` | `(substitute new old lst [:test fn])` | 替换匹配元素 |

### 2.3 列表操作

| 函数 | 签名 | 说明 |
|------|------|------|
| `last` | `(last lst)` | 返回最后一个 cons |
| `butlast` | `(butlast lst)` | 返回除最后一个外的列表 |

---

## Phase 3: 字符串增强（低工作量）

| 函数 | 签名 | 说明 |
|------|------|------|
| `string-upcase` | `(string-upcase s)` | 转大写 |
| `string-downcase` | `(string-downcase s)` | 转小写 |
| `string-trim` | `(string-trim s)` | 去除首尾空白 |

**实现**：在 `builtins_string.cpp` 中新增，用 C++ `<algorithm>`/`<cctype>` 实现。

---

## 依赖关系

```
Phase 1 (别名层)     ← 无依赖，可直接做
  ├─ 1.1 first/second/third/rest
  ├─ 1.2 null/atom/consp/listp
  └─ 1.3 nth

Phase 1.5 (形参语法) ← 无依赖，但说明文档建议 Phase 1 之后
  └─ &optional/&key/&rest

Phase 2 (序列函数)   ← 无依赖，可并行于 Phase 1.5
  ├─ 2.1 find/position/count
  ├─ 2.2 remove/remove-if/substitute
  └─ 2.3 last/butlast

Phase 3 (字符串)     ← 无依赖，可并行于 Phase 1
  └─ string-upcase/downcase/trim
```

## 验证方式

1. 每个新增函数至少 1 个 smoke test
2. `ctest --preset debug` 全部通过
3. AI 口头禅式测试：`(first '(10 20 30))` → `10`、`(null nil)` → `#t`、`(nth 1 '(a b c))` → `b`

## 工作量估算

| 阶段 | 文件 | 新增行数 | 估算时间 |
|------|------|----------|----------|
| Phase 1 | 2-3 个 .cpp | ~40 行 | 15-30 分钟 |
| Phase 1.5 | 2-3 个文件 | ~100 行 | 1-2 小时 |
| Phase 2 | 1-2 个 .cpp | ~120 行 | 1 小时 |
| Phase 3 | 1 个 .cpp | ~30 行 | 15 分钟 |
| **合计** | **5-8 个文件** | **~290 行** | **3-4 小时** |
