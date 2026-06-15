# PML 入门指南

PML（Picture Markup Language）是一种给 LLM 用的"代码生图"语言。你用 Lisp 风格的代码描述想要的画面，PML 编译后输出 PNG/GIF 图片。不限定用例——游戏素材、UI 界面、数据可视化、插画、图标，任何图像都可以。

---

## 快速上手

### 安装依赖

```bash
uv sync
```

### 运行 .pml 文件

```bash
uv run pml examples/hello.pml
```

### 交互式 REPL

```bash
uv run pml
pml> (+ 1 2)
3
pml> (exit)
```

### 命令行选项

```bash
uv run pml my.pml --json        # 输出 JSON 结果
uv run pml my.pml -o ./output   # 指定输出目录
uv run pml my.pml --watch       # 监听文件修改自动重跑
uv run pml --help               # 查看全部选项
```

---

## 基础语法

### 数据

```
42          → 数字
"你好"      → 字符串
#t  #f      → 真／假
hello       → 符号（类似变量名）
:color      → 关键字（用作参数名）
```

### 函数调用 = 前缀表达式

```
(+ 1 2)          → 3
(* 3 4)          → 12
(/ 10 2)         → 5
(string? "hi")   → #t
```

### 定义变量

```
(define x 42)
(define name "张三")
(define pi 3.14159)
```

### 定义函数

```
(define (square x) (* x x))
(square 5)   → 25
```

### 条件判断

```
(if (> x 10) "大" "小")

(cond
  ((< n 0) "负数")
  ((= n 0) "零")
  (else "正数"))
```

### 列表 — 注意陷阱！

```lisp
'(1 2 3)         ← 这是数据列表（正确用法）
(1 2 3)          ← 这是函数调用！会把 1 当函数调，会报错
```

列表操作：
```
(car '(1 2 3))          → 1        （取第一个）
(cdr '(1 2 3))          → (2 3)    （取剩下的）
(cons 1 '(2 3))         → (1 2 3)  （在前面加一个）
(list 1 2 3)            → (1 2 3)  （创建一个列表）
```

---

## 画图

### 基本图形

```lisp
; 创建画布
(canvas 300 200 :bg "#f5f5f5")

; 添加图形（全部支持 :fill :stroke :stroke-width）
(add (rect 20 20 80 60 :fill "#3498db"))
(add (circle 180 60 35 :fill "#e74c3c"))
(add (ellipse 260 100 25 40 :fill "#2ecc71"))
(add (line 20 150 280 150 :stroke "#9b59b6" :stroke-width 3))

; 渲染输出
(render "output.png")
```

### 画一个角色精灵

```lisp
; 创建精灵画布（带自动居中）
(sprite-canvas 128 256 :bg "transparent")

; 组装角色
(define hero
  (character
    :expression "happy"
    :style 'cel))

(add hero)
(render "hero.png")
```

### 变换

```
(translate dx dy)    → 平移
(scale sx sy)        → 缩放
(rotate angle)       → 旋转（弧度）
(compose t1 t2)      → 组合两个变换
```

### Spritesheet（多图合一张）

```lisp
; 把多个精灵排成网格，适合游戏导出
(render-spritesheet "sprites.png"
  (character :expression "happy")
  (character :expression "angry")
  (character :expression "surprised")
  :cols 3
  :cell-width 64
  :cell-height 64)
; 同时输出 .spritesheet.json 记录每帧坐标
```

---

## 动画

```lisp
; 创建画布和物体
(canvas 200 200 :bg "#1a1a2e")
(define ball (circle 30 30 20 :fill "#e74c3c"))

; 注册动画（支持各种缓动函数）
(animate ball 'x 30 170 1.0 :ease 'sine-in-out)
(animate ball 'y 30 170 0.5 :ease 'bounce-out :delay 0)

; 播放并导出 GIF
(play)
(render "bounce.gif" :fps 30)
```

内置缓动：`linear` `sine-in-out` `quad-in` `quad-out` `bounce-out` `elastic-out` 等。

---

## 骨骼 & IK

适合做角色手臂、腿的逆向运动学。

```lisp
; 1. 定义骨骼模板
(defskeleton arm (x y)
  (joint shoulder :pos (0 0)   :length 40.0 :angle 0.0 :min -120 :max 120)
  (joint elbow   :pos (0 40)  :length 35.0 :angle 0.0 :min -150 :max 0)
  (joint wrist   :pos (0 35)  :length 0.0  :angle 0.0))

; 2. 实例化（放在画布的 (50, 180) 位置）
(define arm-inst (instantiate-skeleton arm 50 180))

; 3. 求解 IK（让手腕伸向目标点）
(ik-solve arm-inst 'wrist 95 125 :method 'fabrik :iterations 30)

; 4. 查询关节位置
(joint-position arm-inst 'elbow)   → (x y)
```

---

## 模块系统

把常用代码分到不同文件。

```
;; my-lib.pml
(define (square x) (* x x))
(provide square)
```

```
;; main.pml
(import "my-lib.pml" as lib)
(lib/square 5)   → 25
```

---

## 标准库

| 文件 | 用途 |
|------|------|
| `stdlib/math.pml` | `clamp` `lerp` `normalize` `remap` `distance` `sign` |
| `stdlib/color.pml` | 颜色工具 |
| `stdlib/easing.pml` | 各种缓动函数 |
| `stdlib/shapes.pml` | `centered-rect` `diamond` `arrow` |

---

## MCP 服务器（给 AI Agent 用）

把 PML 封装成 AI 能直接调用的工具（兼容 Claude Desktop、Cursor 等）：

```bash
uv run pml-mcp
```

暴露 5 个工具：
- `execute_pml` — 执行 PML 代码
- `render_sprite` — 渲染精灵为 PNG
- `validate` — 语法检查
- `list_components` — 列出可用组件
- `preview_params` — 查看组件参数

### Claude Desktop 配置

在 `claude_desktop_config.json` 中添加：

```json
{
  "mcpServers": {
    "pml": {
      "command": "uv",
      "args": ["run", "pml-mcp"],
      "cwd": "/path/to/PML"
    }
  }
}
```

---

## 常见坑

### 列表 vs 函数调用
```
(100 200)     ❌ 会把 100 当函数调用
'(100 200)    ✅ (100 200)
(list 1 2 3)  ✅ (1 2 3)
```

### 没有 `cadr`
PML **没有** `cadr`、`caar`、`cddr`。取第二个元素用：
```
(car (cdr '(1 2 3)))   → 2
```

### 骨骼 IK 的 joint 位置
用 `defskeleton` 时，如果加 `:pos` 偏移可能会导致 FABRIK 求解器双重应用位置。遇到问题就把所有 `:pos` 设成 `(0 0)`。

### 渲染前必须要有 canvas
```
(add (circle 50 50 20))       ❌ 没创建画布
(canvas 200 200)               ✅ 先创建画布
(add (circle 50 50 20))       ✅ 再加物体
(render "out.png")
```

---

## 完整示例

```lisp
;; 一个完整的角色精灵动画
(canvas 200 200 :bg "#1a1a2e")

(define logo (circle 100 100 40
               :fill "#f1c40f"
               :stroke "#e67e22"
               :stroke-width 3))

(animate logo 'x 60 140 0.8 :ease 'sine-in-out)
(animate logo 'x 140 60 0.8 :ease 'sine-in-out :delay 0.8)
(animate logo 'y 60 140 0.5 :ease 'quad-out :delay 0)
(animate logo 'y 140 60 0.5 :ease 'quad-in :delay 0.5)
(animate logo 'fill "#f1c40f" "#e74c3c" 0.5)
(animate logo 'fill "#e74c3c" "#f1c40f" 0.5 :delay 0.5)

(play)
(render "combined.gif" :fps 30)
```

更多示例见 `examples/` 目录。

---

## 系统要求

- Python ≥ 3.10
- Pillow ≥ 12.2.0
- `uv` 包管理器（[安装指南](https://docs.astral.sh/uv/#installation)）

---

## 许可证

[MIT](../LICENSE)
