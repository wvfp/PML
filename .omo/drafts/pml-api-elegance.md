# PML API 优雅化方案

> 本计划在原有草案基础上补充了：每项提案的完整语法设计、文件级实施分解、测试规范、依赖关系图、迁移手册、风险评估、非目标以及验收标准。

---

## 1. 现状痛点（已确认）

### 对比代码库

| 痛点 | 代码片段 | 严重度 |
|------|---------|--------|
| 变换管道繁琐 | `(with-transform square (compose (translate 320 320) (compose (rotate 30) (scale 1.5 1.5))))` | 高 |
| 图层合成零散 | `define comp -> define bg -> define red-box -> set! comp -> composition-render`，5步+ | 高 |
| 无循环语法 | `map` / `for-each` / 递归 | 中 |
| 无解构 | `car` / `cadr` 拆解 `iso-to-screen` 返回值 | 中 |
| 变换 API 分裂 | `translate-object` / `rotate-object` / `scale-object` 两套 | 低 |
| `let` 默认并行 | 与主流 Scheme 一致，但违背直觉 | 低 |
| 滤镜链嵌套 | `(compose-filters (blur 4) (compose-filters (drop-shadow ...) (color-filter ...)))` | 中 |

### 现有可复用基础设施

- **宏系统**：`defmacro` + `quasiquote`/`unquote` — 非 hygiene 但足够
- **内置 stdlib**：13 个嵌入式 `.pml` 文件，总计
- **变换系统**：`AffineTransform` + `compose` + `with-transform`
- **图形对象**：不可变 + 链式 `with_*` 方法
- **图层系统**：`Layer` + `Composition` + `Compositor` 完整
- **动画**：`Timeline` + `Animation` + `animate`
- **着色器**：`noise-fractal` / `noise-turbulence`

### 关键文件位置

| 文件 | 路径 |
|------|------|
| `let` / `let*` 实现 | `src/pml/evaluator/evaluator.cpp:1068-1170` |
| `let` / `let*` 注册 | `src/pml/evaluator/evaluator.cpp:1703-1704` |
| 内置函数注册 | `src/pml/evaluator/builtins.cpp` (+ 20+ 分类文件) |
| Stdlib 嵌入 | `src/pml/core/embedded_stdlib.h` (生成于 `tools/embed_stdlib.py`) |
| Stdlib 源目录 | `stdlib/` **已删除** — 需重建 |
| 示例脚本 | `examples/` (60+ 文件，12 个分类) |

> `stdlib/` 源目录在嵌入到 C++ 后已被删除。修改 stdlib 需重建该目录，编辑 `.pml`，然后运行 `python tools/embed_stdlib.py`。

---

## 2. 语法设计（每项提案的完整定义）

### 2.1 `->` 线程宏（变换管道）

```pml
;; 当前 —— 嵌套 compose，极不直观
(with-transform square
  (compose (translate 320 320)
    (compose (rotate 30) (scale 1.5 1.5))))

;; 之后 —— Clojure 风格线程宏
(->> square
  (translate 320 320)
  (rotate 30)
  (scale 1.5 1.5))

;; 含义：(with-transform square (compose (translate 320 320) (compose (rotate 30) (scale 1.5 1.5))))
```

**宏定义**（在 `stdlib/threading.pml` 中）：

```pml
(defmacro -> (val . forms)
  "Thread val as the FIRST argument through each form."
  (if (null? forms)
      val
      (let* ((f (car forms))
             (rest (cdr forms)))
        `(-> (,(car f) ,val ,@(cdr f)) ,@rest))))

(defmacro ->> (val . forms)
  "Thread val as the LAST argument through each form."
  (if (null? forms)
      val
      (let* ((f (car forms))
             (rest (cdr forms)))
        `(->> (,(car f) ,@(cdr f) ,val) ,@rest))))
```

### 2.2 `for` 循环宏

```pml
;; 当前 —— 手写递归或 map
(define (draw-circles n)
  (if (= n 0) '()
    (begin
      (add (circle (* n 40) 200 20 :fill "#3498DB"))
      (draw-circles (- n 1)))))

;; 之后 —— 简洁迭代
(for (i 1 10 1)
  (add (circle (* i 40) 200 20 :fill "#3498DB")))

;; 语法：(for (var start end [step]) body...)
;; step 默认为 1
```

**宏定义**（在 `stdlib/iteration.pml` 中）：

```pml
(defmacro for (bindings . body)
  (let ((var (car bindings))
        (start (cadr bindings))
        (end (caddr bindings))
        (step (if (null? (cdddr bindings)) 1 (cadddr bindings))))
    `(letrec ((loop (lambda (,var)
                      (when (<= ,var ,end)
                        ,@body
                        (loop (+ ,var ,step))))))
       (loop ,start))))
```

### 2.3 `defn` / `defconst` 简写

```pml
;; 当前
(define (greet name) (string-append "Hello, " name))
(define PI 3.14159)

;; 之后
(defn greet (name) (string-append "Hello, " name))
(defconst PI 3.14159)
```

**宏定义**（在 `stdlib/sugar.pml` 中）：

```pml
(defmacro defn (name params . body)
  `(define (,name ,@params) ,@body))

(defmacro defconst (name value)
  `(define ,name ,value))
```

### 2.4 `let-dest` 解构

```pml
;; 当前
(let* ((result (iso-to-screen 10 20))
       (screen-x (car result))
       (screen-y (cadr result)))
  ...)

;; 之后
(let-dest ((screen-x screen-y) (iso-to-screen 10 20))
  ...)

;; 支持嵌套列表和向量
(let-dest ((x y z) '(1 2 3)) (+ x y z))  ;; -> 6
(let-dest ((a (b c) d) (list 1 (list 2 3) 4))
  (list a b c d))  ;; -> (1 2 3 4)
```

> **注意**：完整解构需要 C++ 层支持（递归模式匹配）。最小可行版本仅支持平面列表解构，纯宏实现。

### 2.5 `with-composition` 图层宏

```pml
;; 当前 —— 5 步 + 重复 set!/render
(define comp (make-composition "basic-layers" 400 300 :bg "#ECF0F1"))
(define bg (make-layer "bg" (rect 0 0 400 300 :fill "#F8F9FA")))
(define red-box (make-layer "red-box" (rect 40 40 120 120 :fill "#E74C3C" :rx 8) :opacity 0.8))
(define blue-circle (make-layer "blue-circle" (circle 0 0 60 :fill "#3498DB") :offset '(260 150)))
(set! comp (composition-add comp bg red-box blue-circle))
(composition-render comp "01_basic_layers.png")

;; 之后 —— 一行搭建完整 composition
(with-composition ("basic-layers" 400 300 :bg "#ECF0F1")
  (layer "bg" (rect 0 0 400 300 :fill "#F8F9FA"))
  (layer "red-box" (rect 40 40 120 120 :fill "#E74C3C" :rx 8) :opacity 0.8)
  (layer "blue-circle" (circle 0 0 60 :fill "#3498DB") :offset '(260 150))
  (render "01_basic_layers.png"))
```

### 2.6 `->filter` 滤镜管道

```pml
;; 当前
(let ((f (compose-filters (blur 4)
          (compose-filters (drop-shadow :dx 3 :dy 3 :blur 6 :color "#00000040")
                           (color-filter :brightness 1.2 :contrast 0.9)))))
  (make-layer "filtered" shape :filter f))

;; 之后
(make-layer "filtered" shape
  :filter (->filter (blur 4)
                    (drop-shadow :dx 3 :dy 3 :blur 6 :color "#00000040")
                    (color-filter :brightness 1.2 :contrast 0.9)))
```

### 2.7 `range` 重载

```pml
;; 当前：只支持 (range start end) 和 (range start end step)
(range 0 10)       ;; -> (0 1 2 ... 9)
(range 0 10 2)     ;; -> (0 2 4 6 8)

;; 之后：支持单参数 (range end)
(range 10)         ;; -> (0 1 2 ... 9)  [新增]
(range)            ;; -> error: expected 1-3 args
```

### 2.8 `enumerate` / `zip`

```pml
;; enumerate
(enumerate '(a b c))  ;; -> ((0 a) (1 b) (2 c))

;; zip
(zip '(1 2 3) '(a b c))       ;; -> ((1 a) (2 b) (3 c))
(zip '(1 2) '(a b c))         ;; -> ((1 a) (2 b))   最短截断
(zip '(1 2 3) '(a b) '(x y z)) ;; -> ((1 a x) (2 b y))
```

### 2.9 `get` / `set!` 通用多态

```pml
;; 统一访问接口，类型分派
(get my-list 0)        ;; list-ref
(get my-vec 2)         ;; vector-ref
(get my-hash 'key)     ;; hash-ref

(set! lst 0 new-val)   ;; list-set
(set! vec 2 new-val)   ;; vector-set!
(set! hash 'k new-val) ;; hash-set!
```

### 2.10 `tilemap-fill!` / `tilemap-noise-fill!`

```pml
;; 批量填充
(tilemap-fill! tm grass-tile 0 15 0 10)    ;; 填充矩形区域
(tilemap-noise-fill! tm [grass sand stone]  ;; 噪声生成地形
  :seed 42 :octaves 4 :gain 0.5)
```

### 2.11 `isometric-canvas`

```pml
;; 封装 iso-to-screen 坐标转换 + 瓦片渲染循环
(isometric-canvas 10 8 64
  (lambda (iso-x iso-y screen-x screen-y)
    (add (tile ... :x screen-x :y screen-y))))
```

### 2.12 关键字短别名

```pml
;; 之后：支持缩写
(rect 0 0 100 100 :f "#E74C3C" :s "#333" :sw 2)
;; 等价于：:fill :f   :stroke :s   :stroke-width :sw
```

**实现方式**：在解析器中将关键字别名映射表静态注册，在 `read` 阶段展开。

---

## 3. 实施 Wave 分解

```text
Wave 1 [stdlib 宏层] -----+--- Wave 2 [C++ 内置函数增强] ----+--- Wave 3 [Breaking Changes]
    依赖：无                 依赖：Wave 1 完成                  依赖：Wave 1+2 完成
    风险：无                 风险：低                          风险：中高
```

### Wave 1: Stdlib 宏层（零 C++ 改动）

**目标**：纯 PML 宏实现，嵌入到 stdlib 中，立即可用。

#### 需创建的文件

| 文件 | 内容 | 依赖 |
|------|------|------|
| `stdlib/threading.pml` | `->`, `->>` 线程宏 | 无 |
| `stdlib/iteration.pml` | `for` 循环宏 | 无 |
| `stdlib/sugar.pml` | `defn`, `defconst` | 无 |
| `stdlib/composition.pml` | `with-composition`, `layer` | 无 |
| `stdlib/filter.pml` | `->filter` 滤镜管道宏 | 无 |
| `stdlib/destructure.pml` | `let-dest` 解构宏（平面模式） | 无 |
| `stdlib/range.pml` | `range` 重载宏包装 | `math.pml` |

#### 需删除的文件

| 文件 | 理由 |
|------|------|
| `stdlib/sprites/styles.pml` | 未在示例中 import |
| `stdlib/sprites/body.pml` | 未在示例中 import |
| `stdlib/sprites/eyes.pml` | 未在示例中 import |
| `stdlib/sprites/hair.pml` | 未在示例中 import |
| `stdlib/sprites/items.pml` | 未在示例中 import |
| `stdlib/sprites/outfit.pml` | 未在示例中 import |
| `stdlib/sprites/scene.pml` | 未在示例中 import |
| `stdlib/sprites/ui.pml` | 未在示例中 import |
| `stdlib/sprites/character.pml` | 保留（需确认是否被引用） |

#### 测试规格（新建 `tests/wave1_macros_test.cpp`）

| 测试用例 | 预期 |
|----------|------|
| `(-> 10 (+ 2) (* 3))` | `36` |
| `(->> (list 1 2 3) (map (lambda (x) (* x 2))))` | `(2 4 6)` |
| `(for (i 1 5 1) (collect i))` | `(1 2 3 4 5)` |
| `(defconst PI 3.14) -> (* PI 2)` | `6.28` |
| `(defn double (x) (* x 2)) -> (double 5)` | `10` |
| `(with-composition (\"test\" 100 100) (layer \"a\" (circle 50 50 10)) (render \"out.png\"))` | 展开正确 |
| `(->filter (blur 4) (color-filter :brightness 1.2))` | 展开为 compose-filters |

#### 实施步骤

1. 创建 `stdlib/` 目录结构
2. 从 `embedded_stdlib.h` 反解恢复现有 `.pml` 文件
3. 按上表新建宏文件
4. 删除未使用的 sprites 文件
5. 运行 `python tools/embed_stdlib.py` 重新生成 `embedded_stdlib.h`
6. 构建测试，验证宏展开

### Wave 2: C++ 内置函数增强

**目标**：在 C++ builtins 层添加新的内置函数。

#### C++ 改动

| 文件 | 改动 |
|------|------|
| `builtins_containers.cpp` | 新增 `builtin_enumerate`, `builtin_zip` |
| `builtins_containers.h` | 声明 |
| `builtins_list.cpp` | `range` 重载：添加单参数分支 |
| `builtins_list.h` | 声明 |
| `tilemap_builtins.cpp` | 新增 `builtin_tilemap_fill`, `builtin_tilemap_noise_fill` |
| `tilemap_builtins.h` | 声明 |
| `canvas_builtins.cpp` | 新增 `builtin_isometric_canvas` |
| `canvas_builtins.h` | 声明 |
| **新建** `builtins_polymorphic.cpp` | 通用 `get` / `set!` 内置函数 |
| **新建** `builtins_polymorphic.h` | 声明 |
| `builtins.cpp` | 添加新注册函数调用 |
| `builtins.h` | 添加新注册函数声明 |

#### 函数签名设计

```cpp
// enumerate: (enumerate list) -> ((0 a) (1 b) ...)
Result<Value> builtin_enumerate(const std::vector<Value>& args, Environment& env);

// zip: (zip list1 list2 ...) -> ((a1 b1 ...) (a2 b2 ...))
Result<Value> builtin_zip(const std::vector<Value>& args, Environment& env);

// range 重载：新增单参数 (range end) 等价于 (range 0 end 1)
// 修改现有 builtin_range 以支持参数数量检查

// get: (get container index) -> 多态分派
Result<Value> builtin_get(const std::vector<Value>& args, Environment& env);

// set!: (set! container index value) -> 多态分派
Result<Value> builtin_set_bang(const std::vector<Value>& args, Environment& env);

// tilemap-fill!: (tilemap-fill! tm tile col-start col-end row-start row-end [layer])
Result<Value> builtin_tilemap_fill(const std::vector<Value>& args, Environment& env);

// tilemap-noise-fill!: (tilemap-noise-fill! tm tiles :seed N :octaves N :gain N [layer])
Result<Value> builtin_tilemap_noise_fill(const std::vector<Value>& args, Environment& env);

// isometric-canvas: (isometric-canvas cols rows tile-width fn)
Result<Value> builtin_isometric_canvas(const std::vector<Value>& args, Environment& env);
```

#### 测试规格

| 测试用例 | 预期 |
|----------|------|
| `(enumerate '(a b c))` | `((0 a) (1 b) (2 c))` |
| `(zip '(1 2) '(a b))` | `((1 a) (2 b))` |
| `(zip '(1 2) '(a b c))` | `((1 a) (2 b))`（最短截断） |
| `(range 5)` | `(0 1 2 3 4)` |
| `(range 0 5)` | `(0 1 2 3 4)`（保留原行为） |
| `(range 1 10 3)` | `(1 4 7)`（保留原行为） |
| `(get '(a b c) 0)` | `a` |
| `(get #(10 20 30) 1)` | `20` |
| `(get (hash 'x 42) 'x)` | `42` |

### Wave 3: Breaking Changes + 适配

**目标**：`let` 语义变更、示例迁移、文档更新。

#### `let` 语义变更

**C++ 改动**（`src/pml/evaluator/evaluator.cpp`）：

| 位置 | 当前 | 改为 |
|------|------|------|
| L1068 | `eval_let` — 并行绑定 | `eval_let` — 串行绑定（原 `eval_let_star` 的逻辑） |
| L1124 | `eval_let_star` — 串行绑定 | `eval_let_par` — 并行绑定（原 `eval_let` 的逻辑） |
| L1703 | `{"let", eval_let}` | 不变（`eval_let` 现在做串行） |
| L1704 | `{"let*", eval_let_star}` | `{"let-par", eval_let_par}` — 新名字 |
| L1705 | `{"letrec", eval_letrec}` | 不变 |

**替换模式**：
- 现有 `(let ...)` — 保持原样（语义变成串行，通常不影响正确性）
- 需要并行绑定的情况 — 改为 `(let-par ...)`
- `(let* ...)` — 保持或改为 `(let ...)`（因为 `let` 现在就是串行）

#### 迁移计划

1. 自动扫描所有 `(let` 和 `(let*` 出现位置
2. 逐文件审查语义是否正确
3. 更新测试以验证新行为
4. 更新文档

#### 影响范围

| 文件 | `let` 使用数 | 是否需要并行？ | 操作 |
|------|-------------|--------------|------|
| `examples/12-edge-perturb/05_showcase.pml` | 1 | 否 | 保留 `let` |
| `examples/09-projects/05_doubao_logo/arc.pml` | 1 | 否 | 保留 `let` |
| `examples/09-projects/11_anime_wilderness/isometric.pml` | 2 | 否 | 保留 `let` |
| `stdlib/*.pml` | 需检查 | 需评估 | 逐行审查 |
| `Project/*.pml` | ~230 | 可选 | 可选迁移 |

### Wave 4: 关键字别名（解析器层）

**目标**：支持 `:f` 代替 `:fill`，`:s` 代替 `:stroke` 等。

**实现方式**：在词法分析器或解析器阶段维护关键字别名映射。

| 别名 | 展开 |
|------|------|
| `:f` | `:fill` |
| `:s` | `:stroke` |
| `:sw` | `:stroke-width` |
| `:op` | `:opacity` |
| `:bg` | `:background` |
| `:fn` | `:font-name` |
| `:fs` | `:font-size` |

---

## 4. 依赖关系与任务顺序

```text
Wave 1: 重建 stdlib/ + 宏文件 + embed + 测试
  |   (各宏文件可并行)
  v
Wave 2: enumerate/zip + range + get/set! + tilemap + isometric (可并行)
  |
  v
Wave 3: let 语义交换 + 迁移 + 测试适配
  |
  v
Wave 4: 关键字别名 (独立)
```

### 并行决策

| 任务 | 可并行？ | 理由 |
|------|---------|------|
| Wave 1 各宏文件 | 是 | 文件间无交叉依赖 |
| Wave 2 enumerate/zip + range | 是 | 不同 builtins 文件 |
| Wave 2 get/set! + tilemap + isometric | 是 | 独立功能单元 |
| Wave 1 vs Wave 2 | 否 | Wave 1 确认宏系统健康后再进 C++ |
| Wave 3 vs Wave 4 | 是 | 互不依赖 |

---

## 5. 风险评估与缓解

| 风险 | 概率 | 影响 | 缓解措施 |
|------|------|------|---------|
| `->` 宏与现有 `->` 符号冲突 | 中 | 高 | 搜索确认 `->` 是否已绑；备用名 `thread` / `thread->` |
| `for` 宏名与内置函数冲突 | 低 | 高 | 搜索确认内置绑表中无 `for` |
| `let` 语义变更破坏 `Project/*.pml` | 高 | 中 | `Project/` 是用户实验目录，不承诺向后兼容 |
| 关键字别名导致解析歧义 | 低 | 高 | 别名仅在关键字形参语境中展开 |
| `stdlib/` 重建时丢失原始文件 | 中 | 高 | 从 `embedded_stdlib.h` 用 Python 逆向提取 |
| 宏展开导致调试困难 | 中 | 低 | 保持宏简单（<=20 行），为复杂宏添加 `macroexpand-1` 测试 |

### 回滚策略

- **Wave 1**：回退 `embedded_stdlib.h` 历史版本
- **Wave 2**：回退各 builtins 文件 + 重新注册
- **Wave 3**：`let` 语义变更只需恢复 `evaluator.cpp` 的 L1068-L1170 和 L1703-L1705
- **Wave 4**：移除关键字别名代码

---

## 6. 非目标（显式排除）

| 项目 | 理由 |
|------|------|
| `match` 模式匹配 | 需要完整 C++ 模式匹配引擎，超出优雅化范围 |
| `loop`/`recur` 尾递归优化 | 需要求值器改造；TCO 是独立项目 |
| 数值 `for` 编译为 builtin | 当前纯宏实现足够 |
| `let` 语义变更后删除 `let*` | 保留 `let*` 别名确保向后兼容 |
| 完整 hygiene 宏系统 | 需要 C++ 重写宏展开器 |
| 3D API 优雅化 | 3D 功能使用频率低 |
| 骨骼/IK API 解封 | 已封存，除非特殊需求 |
| CLI 重构（`project.pml` 等） | 属于模块系统增强，独立范围 |
| 性能优化 | 本计划聚焦 API 表面，不涉及渲染性能 |

---

## 7. 验收标准

### Wave 1 完成标志

- [ ] `stdlib/` 目录重建，包含所有 13+ 个 `.pml` 文件
- [ ] 所有新宏文件包含在 `embedded_stdlib.h` 中
- [ ] 构建通过（`cmake --build --preset debug`）
- [ ] 宏展开测试通过（最少 10 个测试用例）
- [ ] 每个宏至少在一个示例中实际运行通过

### Wave 2 完成标志

- [ ] LSP 诊断清洁（所有修改的 C++ 文件零错误）
- [ ] 构建通过
- [ ] `enumerate`、`zip`、`range`（单参数）、`get`/`set!` 的测试通过
- [ ] `tilemap-fill!`、`tilemap-noise-fill!` 的冒烟测试通过
- [ ] 至少有 1 个示例使用新的内置函数

### Wave 3 完成标志

- [ ] `let` 做串行绑定，`let-par` 做并行绑定
- [ ] `examples/` 中所有文件按需迁移
- [ ] `stdlib/` 中所有文件按需迁移
- [ ] 现有测试适配后全部通过
- [ ] `let*` 仍然可用且语义不变

### Wave 4 完成标志

- [ ] 关键字别名在解析器/宏层生效
- [ ] `(rect 0 0 100 100 :f "#E74C3C")` 等价于 `(rect 0 0 100 100 :fill "#E74C3C")`
- [ ] 构建通过，无歧义

### 全局完成

- [ ] 所有 4 个 Wave 的验收条件满足
- [ ] 至少 5 个现有示例被重写为新语法
- [ ] `cmake --build --preset debug` 通过
- [ ] `ctest --preset debug` 所有测试通过（或明确记录预存失败）

---

## 8. 附录

### A. 代码库关键行号（修改前）

| 功能 | 文件 | 行号 |
|------|------|------|
| `let` 并行实现 | `evaluator.cpp` | 1068-1120 |
| `let*` 串行实现 | `evaluator.cpp` | 1124-1170 |
| `let`/`let*`/`letrec` 注册 | `evaluator.cpp` | 1703-1705 |
| `range` 内置函数 | `builtins_list.cpp` | 查找 `builtin_range` |
| 内置函数总注册 | `builtins.cpp` | 全部 |
| 容器内置函数 | `builtins_containers.cpp` | 全部 |
| 瓦片图内置函数 | `tilemap_builtins.cpp` | 全部 |
| Stdlib 生成 | `tools/embed_stdlib.py` | 全部 |
| Stdlib 嵌入 | `src/pml/core/embedded_stdlib.h` | 全部（自动生成） |

### B. 快速参考：实施顺序建议

```text
 1. 重建 stdlib/ + 恢复现有文件（基础）
 2. defn / defconst 宏（最简单，立即有正面影响）
 3. -> / ->> 线程宏（核心痛点之一）
 4. for 循环宏（核心痛点之一）
 5. with-composition / layer 宏（核心痛点之一）
 6. ->filter 滤镜宏
 7. let-dest 解构宏
 8. 重新生成 embedded_stdlib.h + 测试
    ------- Wave 1 完成 -------
 9. enumerate / zip C++ 内置函数
10. range 重载
11. get / set! 多态
12. tilemap-fill! + noise-fill!
13. isometric-canvas
    ------- Wave 2 完成 -------
14. let 语义交换（最后一个改动）
15. 示例逐个迁移验证
16. 文档更新
    ------- 全部完成 -------
17. 关键字别名（独立，不影响以上）
```

### C. 术语表

| 术语 | 含义 |
|------|------|
| Stdlib | 标准库 `.pml` 文件，嵌入到 C++ 中的 byte array |
| Builtin | C++ 实现的内置函数，通过 `def()` 注册 |
| 线程宏 | Clojure 风格的 `->` / `->>`，将值作为第一个/最后一个参数传入下一函数 |
| 解构 | 从列表/向量中提取元素并绑定到变量名的模式匹配 |
| 关键字别名 | `:fill` 的缩写形式 `:f` |

### D. 待删除的无用 sprites 文件

以下 8 个文件被嵌入但实际未在任何示例中 `import`：
- `sprites/body.pml` / `sprites/eyes.pml` / `sprites/hair.pml`
- `sprites/items.pml` / `sprites/outfit.pml` / `sprites/scene.pml`
- `sprites/styles.pml` / `sprites/ui.pml`
- `sprites/character.pml`（保留 — 需确认是否被使用）
