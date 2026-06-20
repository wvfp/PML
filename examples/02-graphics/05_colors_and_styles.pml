; 示例 5：颜色与样式
; 展示 color/rgb、color/rgba、fill、stroke、stroke-width、no-fill、no-stroke。

(set-backend! "skia")
(canvas 520 300 :bg "#FFFFFF")

; 第一行：纯色条，使用 color/rgb 生成颜色
(add (rect 20 40 90 40 :fill (color/rgb 255 99 71)))
(add (rect 125 40 90 40 :fill (color/rgb 60 179 113)))
(add (rect 230 40 90 40 :fill (color/rgb 30 144 255)))
(add (rect 335 40 90 40 :fill (color/rgb 255 215 0)))

; 第二行：半透明叠加条，使用 color/rgba
(add (rect 125 100 240 40 :fill (color/rgba 138 43 226 128)))
(add (rect 230 100 240 40 :fill (color/rgba 255 69 0 128)))

; 第三行：描边样式对比
; 直接通过关键字设置
(add (rect 20 160 120 40
           :fill "#E9ECEF"
           :stroke "#495057"
           :stroke-width 4))

; 用 fill / stroke 函数修改样式
(add (stroke (fill (rect 160 160 120 40) "#E9ECEF") "#212529"))

; 用 stroke-width 函数加粗描边
(add (stroke-width (rect 300 160 120 40
                            :fill "#E9ECEF"
                            :stroke "#F76707")
                   6))

; 第四行：无填充 / 无描边
(add (no-fill (rect 20 220 110 40
                     :stroke "#40C057"
                     :stroke-width 3)))

(add (no-stroke (rect 145 220 110 40 :fill "#339AF0")))

; 右下角：用半透明圆叠成一个简单色环示意
(define cx 430)
(define cy 150)
(add (circle (- cx 25) cy 30 :fill (color/rgb 255 0 0)))
(add (circle (+ cx 25) cy 30 :fill (color/rgb 0 255 0)))
(add (circle cx (- cy 25) 30 :fill (color/rgb 0 0 255)))
(add (circle cx (+ cy 25) 30 :fill (color/rgb 255 255 0)))
(add (circle cx cy 30 :fill (color/rgba 255 255 255 160)))

(render "05_colors_and_styles.png")
