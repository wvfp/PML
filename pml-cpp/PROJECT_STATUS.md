# PML C++23 — Project Status

**Version:** 0.1.0  
**Commit:** `244fddc` (2026-06-17)  
**Branch:** master  
**Python reference:** `/root/PML` (PML v0.1.0)

---

## TL;DR

C++23 端口已经完成。223 项测试全部通过，构建成功，渲染管线可用。

| 指标 | 数值 |
|------|------|
| 源文件 | 80+ 个（.cpp / .h） |
| 代码行数 | ~18,000 |
| 测试用例 | 223个（44个测试套件） |
| 内置函数 | ~160个 |
| 构建目标 | 17个（10个静态库 + 3个可执行文件 + 测试） |

---

## 总体完成情况

原计划36项任务已全部完成：

| Wave | 任务 | 状态 |
|------|------|------|
| Wave 1 基础 | T1-T6 CMake, 核心类型, 错误, 环境, 变换, stdlib嵌入 | ✅ |
| Wave 2 前端 | T7-T9 词法分析, 语法分析, 宏展开 | ✅ |
| Wave 3 图形 | T10-T14 GraphicObject, Canvas, 后端ABC, 渲染, 颜色 | ✅ |
| Wave 4 求值 | T15-T17 GIF导出, 求值器, 内置函数 | ✅ |
| Wave 5 支持 | T18-T22 模块系统, 样式, 调色板, 缓动, 动画 | ✅ |
| Wave 6 骨架 | T23-T25 骨骼类型, FABRIK, CCD | ✅ |
| Wave 7 集成 | T26-T29 皮肤绑定, 精灵表, 后端内置函数, Skia 着色器 | ✅ |
| Wave 8 CLI | T30-T33 PMLRuntime API, CLI, MCP服务器, stdlib加载器 | ✅ |
| Wave 9 测试 | T34-T36 Google测试套件, 校验脚本, 行为差异文档 | ✅ |

---

## 架构

```
src/pml/
├── core/          types, error, embedded_stdlib        ← 运行时类型系统
├── frontend/      lexer, parser, expander               ← 编译前端
├── evaluator/     evaluator, builtins, environment      ← 求值器 + 内置函数
├── graphics/      transform, objects, canvas, render    ← 图形核心
├── backend/       ABC, registry, null, skia, gif, color ← 渲染后端
├── sprites/       style, palette, registry/components   ← 精灵系统
├── animation/     easing, timeline                      ← 动画
├── skeleton/      skeleton, ik_*, skin_binding          ← 骨骼 + IK
├── module/        module_loader, embedded_stdlib        ← 模块系统
├── api/           PMLRuntime facade                     ← API门面
├── cli/           main, repl                             ← CLI入口
└── mcp/           mcp_server                            ← MCP服务器
```

## 构建

```bash
# 依赖（apt）— Cairo 后端可选；Skia 使用预编译库
sudo apt install libgtest-dev nlohmann-json3-dev libgif-dev libpng-dev

# Windows 预设（默认使用 G:/Project/skia 预编译库）
cmake --preset debug
cmake --build --preset debug

# 运行
.\build\debug\src\pml\cli\Debug\pml.exe file.pml
.\build\debug\src\pml\cli\Debug\pml.exe --json file.pml

# 测试
.\build\debug\tests\Debug\pml-tests.exe         # 223 项
.\build\debug\tests\Debug\pml-builtins-smoke.exe # 118 项冒烟测试
```

---

## 渲染后端

| 后端 | 名称 | 能力 | 状态 |
|------|------|------|------|
| NullBackend | `"null"` | RasterCPU | ✅ 始终编译 |
| SkiaBackend | `"skia"` | RasterCPU, RasterGPU, Shaders, AnimationGIF, Spritesheet | ✅ 默认后端（预编译 Skia） |
| CairoBackend | `"cairo"` | RasterCPU, FontRendering, LoadPNG | ⚪ 可选（`PML_BUILD_CAIRO=ON`） |

Skia 是默认渲染后端，支持：
- 基本图形：circle, rect, ellipse, line, polygon, path
- SVG 路径解析（M/L/H/V/C/S/Q/T/A/Z）
- 文本渲染（Skia shaper）
- PNG / GIF 输出，含多帧动画 GIF
- SkSL 着色器（`shader` / `apply-shader!`）
- 精灵表合成（`render-spritesheet`）

---

## 测试覆盖

| 测试文件 | 测试数 | 覆盖范围 |
|----------|--------|----------|
| test_lexer.cpp | 16 | 整数、浮点、字符串、符号、布尔值、关键字、括号、注释、quote语法糖 |
| test_parser.cpp | 14 | 原子类型、简单/嵌套/空列表、多个顶层表达式、括号匹配 |
| test_evaluator.cpp | 28 | 算术、define/set!、lambda/递归/闭包、if/cond/and/or、let、car/cdr/cons/map/filter、类型谓词 |
| test_graphics.cpp | 22 | AffineTransform、GraphicObject、Canvas、样式、调色板 |
| test_modules.cpp | 20 | 宏（defmacro、macroexpand、gensym）、letrec、高阶函数 |
| test_animation.cpp | 38 | 缓动函数、Animation、Timeline状态机、apply_modifications |
| test_skeleton.cpp | 37 | 骨骼模板/实例、正运动学、FABRIK、CCD、皮肤绑定 |
| test_api.cpp | 19 | PMLRuntime创建/执行/错误、JSON序列化、宏、持久化 |
| builtins_smoke.cpp | 118 | 所有内置函数的冒烟测试 |

**总计：223项测试（44个套件），全部通过**

---

## 已知限制

1. **字体渲染差异** — Skia vs Pillow 字体渲染视觉上不完全一致，已记录在 BEHAVIOR_DIFFERENCES.md
2. **GIF 调色板** — giflib 与 Pillow 的 ADAPTIVE 调色板可能存在微小差异
3. **Windows watch 模式** — `--watch` 使用 `FindFirstChangeNotification`，暂不支持递归目录监听和文件重命名
4. **模块系统表面** — `import` / `provide` 已工作，但 `module-available?` / `module-list` 等自省接口未暴露
5. **测试覆盖** — C++ 测试覆盖了所有核心模块，但 Python 测试套件的每个细节尚未完全移植

---

## PML DSL 内置函数

| 类别 | 函数 |
|------|------|
| **算术** | +, -, \*, /, modulo, abs, min, max, expt, sqrt, floor, ceiling, round |
| **比较** | =, <, >, <=, >=, eq?, equal?, string=?, string<? |
| **IO** | display, newline, read, string→symbol, symbol→string |
| **列表** | car, cdr, cons, list, append, length, reverse, map, filter, reduce, list-ref |
| **字符串** | string, string-append, string-length, string-ref, substring, string-split |
| **类型谓词** | number?, integer?, float?, string?, boolean?, symbol?, keyword?, list?, procedure?, null?, pair? |
| **变换** | translate, rotate, scale, shear, identity-matrix, compose, matrix-inverse, matrix-apply |
| **图形** | canvas, sprite-canvas, add, circle, rect, ellipse, line, polygon, text, group |
| **样式** | fill, stroke, no-fill, no-stroke, stroke-width |
| **对象变换** | with-transform, translate-object, rotate-object, scale-object |
| **颜色** | color/rgb, color/rgba |
| **渲染** | render, render-set, render-spritesheet |
| **精灵** | define-style, use-style, define-palette, palette-ref |
| **动画** | \_animate, \_play, \_stop, \_pause, \_seek |
| **骨架** | defskeleton, instantiate-skeleton, joint-position, ik-solve, bind-skin |
| **后端** | list-backends, set-backend!, current-backend, backend-capabilities, backend-available? |
| **着色器** | shader, apply-shader!（SkSL / SkRuntimeEffect） |
| **特殊形式** | quote, if, cond, define, lambda, let, let\*, letrec, begin, set!, and, or, do, quasiquote, provide, import, defmacro, macroexpand, assert, gensym |

---

## 文件清单（src/pml）

```
src/pml/core/
├── types.h / types.cpp              ← Expr/Value 变体、Symbol、Keyword、Procedure、Macro
├── error.h / error.cpp              ← 12种错误码、PMLException、Result<T> = expected<T,E>
└── embedded_stdlib.h                ← xxd生成的stdlib嵌入头

src/pml/frontend/
├── lexer.h / lexer.cpp              ← 字符流→Token流（17种TokenType）
├── parser.h / parser.cpp            ← Token→AST（递归下降）
└── expander.h / expander.cpp        ← 宏展开（非卫生）

src/pml/evaluator/
├── environment.h / environment.cpp  ← 作用域链（lookup/define/set!/extend）
├── evaluator.h / evaluator.cpp      ← evaluate() 派发 + 20个特殊形式
├── builtins.h / builtins.cpp        ← ~60个内置函数
├── backend_builtins.h / .cpp        ← 5个后端查询内置函数
├── shader_builtins.h / .cpp         ← 2个着色器内置函数（SkSL）
├── transform_builtins.h / .cpp      ← 8个变换内置函数
└── canvas_builtins.h / .cpp        ← 21个画布/图形/样式/颜色内置函数

src/pml/graphics/
├── transform.h / transform.cpp      ← AffineTransform（6分量矩阵）
├── objects.h / objects.cpp          ← GraphicObject（不可变模式）
├── canvas.h / canvas.cpp            ← Canvas + 全局当前画布
├── render.h / render.cpp            ← 渲染派发 + SVG路径解析
├── color.h                          ← 颜色解析（兼容头）
└── graphic_object.h                 ← 兼容头 → objects.h

src/pml/backend/
├── backend.h                        ← RenderBackend ABC
├── capabilities.h                   ← BackendCap位掩码枚举
├── registry.h / registry.cpp        ← BackendRegistry单例
├── null_backend.cpp                 ← NullBackend（始终编译）
├── color_helpers.h / .cpp           ← 颜色解析（ARGB uint32）
├── skia/skia_backend.h / .cpp       ← SkiaBackend（默认后端）
├── cairo/cairo_backend.h / .cpp     ← CairoBackend（可选）
└── gif/gif_exporter.h / .cpp        ← GIF导出（giflib）

src/pml/sprites/
├── style.h / style.cpp              ← StyleDescriptor + 注册表
├── palette.h / palette.cpp          ← Palette + 管理器
├── registry.h / registry.cpp        ← ComponentRegistry + 组件内置函数
└── components/                      ← body, head, eyes, hair, outfit, items, UI, scene

src/pml/animation/
├── easing.h / easing.cpp            ← 12种缓动函数
└── timeline.h / timeline.cpp        ← Animation + Timeline + 帧钩子

src/pml/skeleton/
├── skeleton.h / skeleton.cpp        ← Joint, SkeletonTemplate, SkeletonInstance
├── ik_fabrik.h / ik_fabrik.cpp      ← FABRIK解算器
├── ik_ccd.h / ik_ccd.cpp            ← CCD解算器
└── skin_binding.h / skin_binding.cpp← 皮肤绑定注册表

src/pml/module/
├── module_loader.h / .cpp           ← ModuleLoader + import/provide
└── embedded_stdlib.h / .cpp         ← stdlib运行时加载器

src/pml/api/
├── api.h / api.cpp                  ← PMLRuntime门面

src/pml/cli/
├── main.cpp                         ← CLI入口（文件执行/REPL/watch/JSON）
└── repl.h / repl.cpp                ← REPL + LineAccumulator

src/pml/mcp/
├── mcp_server.h / mcp_server.cpp    ← JSON-RPC 2.0 MCP服务器
```

---

## 构建目标

| 目标 | 类型 | 描述 |
|------|------|------|
| pml_core | 静态库 | 核心类型、错误、stdlib嵌入 |
| pml_frontend | 静态库 | 词法分析器、语法分析器、宏展开器 |
| pml_evaluator | 静态库 | 求值器、环境、内置函数 |
| pml_graphics | 静态库 | 图形对象、画布、变换、渲染 |
| pml_backend | 静态库 | 后端ABC、注册表、NullBackend、颜色工具 |
| pml_backend_skia | 静态库 | Skia渲染后端 |
| pml_backend_gif | 静态库 | GIF导出 |
| pml_backend_cairo | 静态库 | Cairo渲染后端（可选） |
| pml_sprites | 静态库 | 样式、调色板、组件注册表 |
| pml_animation | 静态库 | 缓动、时间线 |
| pml_skeleton | 静态库 | 骨骼、IK解算器、皮肤绑定 |
| pml_module | 静态库 | 模块加载器、stdlib加载器 |
| pml_api | 静态库 | PMLRuntime门面 |
| pml | 可执行文件 | CLI（文件执行 + REPL + watch） |
| pml-mcp | 可执行文件 | MCP JSON-RPC服务器 |
| pml-tests | 可执行文件 | Google Test（223项） |
| pml-builtins-smoke | 可执行文件 | 内置函数冒烟测试（118项） |
