; 示例 1：基本图形
; 展示 circle、圆角 rect、ellipse、line 的用法。

(set-backend! "skia")
(canvas 400 300 :bg "#F8F9FA")

; 红色描边圆
(add (circle 100 150 50
             :fill "#FF6B6B"
             :stroke "#C92A2A"
             :stroke-width 3))

; 绿色圆角矩形（:rx 控制圆角半径）
(add (rect 180 110 160 90
           :fill "#51CF66"
           :stroke "#2B8A3E"
           :stroke-width 2
           :rx 12))

; 蓝色椭圆
(add (ellipse 320 160 40 25
              :fill "#339AF0"
              :stroke "#1864AB"
              :stroke-width 2))

; 紫色粗线段
(add (line 50 50 350 250
           :stroke "#845EF7"
           :stroke-width 4))

(render "01_circle_rect_ellipse_line.png")
