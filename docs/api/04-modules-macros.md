# 04 — Module System + Macros

## 模块系统

### import — 加载模块

```scheme
(import "path.pml")          ; 自动推断前缀（文件名去掉 .pml）
(import "path.pml" as pref)  ; 自定义前缀（bare `as`，无冒号）
(import "lib/shapes.pml")    ; 相对路径（相对于引用文件）
```

环境中有 `__source_file__` 变量时，模块搜索会相对于该文件所在目录。

### provide — 导出符号

```scheme
; 在模块文件中：
(define my-sprite ...)
(define my-tileset ...)
(provide my-sprite my-tileset)
```

### 访问模块导出

```scheme
; 导入后使用 prefix/symbol 语法：
(import "my-lib.pml" as lib)
(lib/my-sprite)              ; 调用模块中导出的函数
(render lib/my-sprite "out.png")  ; 引用模块中导出的值
```

### 模块查询

| 函数 | 签名 | 说明 |
|------|------|------|
| `module-available?` | `(module-available? path)` | 检查文件是否存在可加载 |
| `module-list` | `(module-list)` | 列出所有已加载的模块名 |
| `module-exports` | `(module-exports module)` | 列出模块导出的符号名列表 |

### 示例

```scheme
; 文件 lib/colors.pml:
(provide red green blue)

(define red   "#e74c3c")
(define green "#2ecc71")
(define blue  "#3498db")

; 文件 main.pml:
(import "lib/colors.pml" as c)
(define banner
  (group
    (rect 0 0 100 30 :fill c/red)
    (rect 0 30 100 30 :fill c/green)
    (rect 0 60 100 30 :fill c/blue)))
(render banner "banner.png")
```

### 模块路径搜索

1. 原路径（绝对路径直接使用）
2. 原路径 + `.pml`
3. 相对于引用文件目录
4. 相对于引用文件目录 + `.pml`

### 注意事项

- 模块文件在首次 `import` 时加载并缓存，重复 `import` 返回缓存的模块
- 循环引用自动检测并报 `circular import error`
- 未 `provide` 的符号对外不可见
- 模块有自己的隔离环境，全局 builtins 可用但定义互不干扰

---

## 宏系统

### define-macro — 传统宏

```scheme
(define-macro (twice expr)
  `(begin ,expr ,expr))

(define-macro (when condition . body)
  `(if ,condition (begin ,@body)))
```

### defmacro — 卫生宏（推荐）

自动对宏体内的变量做重命名以避免意外捕获。

```scheme
(defmacro (swap! a b)
  `(let ((tmp ,a))
     (set! ,a ,b)
     (set! ,b tmp)))

(defmacro (unless condition . body)
  `(if (not ,condition) (begin ,@body)))
```

### macroexpand — 调试宏展开

```scheme
(macroexpand '(when (< x 0) (println "negative")))
; 展开后可见宏生成的代码
```

### gensym — 生成唯一符号

```scheme
(gensym)  → g1
(gensym)  → g2
```

### 使用场景

- **代码生成**: 用宏生成重复的图形构建代码
- **DSL 扩展**: 定义领域特定语法糖
- **性能优化**: 编译时展开减少运行时开销
- **defmacro** 优先：自动处理变量捕获问题
- 宏中 `unquote`（`,`）和 `splice-unquote`（`,@`）组合使用
