; 示例 6：宏（defmacro）
; 使用 defmacro 定义 when/unless 宏，展示宏的展开能力。

; when：条件为真时执行体
(defmacro when (cond . body)
  `(if ,cond (begin ,@body) ()))

; unless：条件为假时执行体
(defmacro unless (cond . body)
  `(if (not ,cond) (begin ,@body) ()))

; 简单宏：定义一个绘制圆点的宏
(defmacro dot (x y color)
  `(add (circle ,x ,y 8 :fill ,color)))

(canvas 400 300 :bg "#F5F5F5")

; 使用 when 宏：条件为真时绘制
(when #t
  (add (rect 40 80 30 30 :fill "#E74C3C" :stroke "#2C3E50" :stroke-width 2))
  (add (rect 140 80 30 30 :fill "#E74C3C" :stroke "#2C3E50" :stroke-width 2))
  (add (rect 240 80 30 30 :fill "#E74C3C" :stroke "#2C3E50" :stroke-width 2)))

; 使用 unless 宏：条件为假时绘制
(unless #f
  (add (circle 55 155 15 :fill "#3498DB" :stroke "#2C3E50" :stroke-width 2))
  (add (circle 155 155 15 :fill "#3498DB" :stroke "#2C3E50" :stroke-width 2))
  (add (circle 255 155 15 :fill "#3498DB" :stroke "#2C3E50" :stroke-width 2)))

; 使用 dot 宏绘制装饰点
(dot 40 220 "#2ECC71")
(dot 90 220 "#2ECC71")
(dot 140 220 "#2ECC71")
(dot 190 220 "#2ECC71")
(dot 240 220 "#2ECC71")
(dot 290 220 "#2ECC71")
(dot 340 220 "#2ECC71")

; 条件为假，不绘制
(when #f
  (add (rect 0 0 400 300 :fill "#000000")))

(render "06_macros.png")
