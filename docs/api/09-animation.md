# 09 — Animation

## 动画系统

基于时间线的动画系统，支持关键帧、并行/串行动画、循环。

### 主要函数

| 函数 | 签名 | 说明 |
|------|------|------|
| `animate` | `(animate graphic :duration sec [:loops bool] . body)` | 定义动画 |
| `play` | `(play [name])` | 播放动画 |
| `stop` | `(stop [name])` | 停止动画 |
| `pause` | `(pause [name])` | 暂停动画 |
| `seek` | `(seek name time)` | 跳转到指定时间 |
| `animation-state` | `(animation-state [name])` | 查询动画状态 |
| `every-frame` | `(every-frame fn duration [:easing fn])` | 每帧回调 |
| `parallel` | `(parallel anim1 anim2 ...)` | 并行播多个动画 |
| `sequence` | `(sequence anim1 anim2 ...)` | 串行播多个动画 |

### 基本用法

```scheme
; 定义动画
(define sprite (rect 0 0 50 50 :fill "#e74c3c"))
(animate sprite :duration 2.0 :loops true
  (sequence
    (every-frame (lambda (t)
      (translate-object sprite (* t 50) 0)) 1.0)
    (every-frame (lambda (t)
      (translate-object sprite 0 (* t 50))) 1.0)))

; 控制
(play)    ; 开始播放
(pause)   ; 暂停
(seek 0)  ; 回到开头
```

### Easing 缓动函数

内置 12 种缓动函数：

| 函数名 | 说明 |
|--------|------|
| `linear` | 匀速 |
| `ease-in` / `ease-out` / `ease-in-out` | 平滑开始/结束 |
| `quad-in` / `quad-out` / `quad-in-out` | 二次缓动 |
| `cubic-in` / `cubic-out` / `cubic-in-out` | 三次缓动 |
| `quart-in` / `quart-out` / `quart-in-out` | 四次缓动 |

```scheme
(every-frame my-fn 2.0 :easing 'ease-out)
```

### 并行动画

```scheme
(parallel
  (animate obj1 :duration 1.0
    (every-frame (lambda (t) (translate-object obj1 (* t 100) 0)) 1.0))
  (animate obj2 :duration 1.0
    (every-frame (lambda (t) (translate-object obj2 0 (* t 100))) 1.0)))
```

### 串行动画

```scheme
(sequence
  (animate obj :duration 1.0
    (every-frame (lambda (t) (translate-object obj (* t 100) 0)) 1.0))
  (animate obj :duration 1.0
    (every-frame (lambda (t) (translate-object obj 0 (* t 100))) 1.0)))
```

### 动画渲染

```scheme
; 渲染动画为 GIF
(let ((c (canvas)))
  (add c my-animated-sprite)
  (render c "anim.gif" :format "GIF" :fps 12))

; 或直接渲染
(render "anim.gif" :format "GIF" :fps 12 :duration 3.0)
```

### 注意事项

- `animate` 返回一个 `Animation` 值
- 时间线全局单例（`g_timeline`），每个 `PMLRuntime` 实例独立
- `every-frame` 的回调接收归一化时间 `t`（0 → 1）
- `:loops true` 在动画结束时自动从头播放
- 动画只在渲染输出时计算帧，不在 REPL 中实时播放
