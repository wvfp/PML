; 示例 3：文字与标签
; 展示 text 原语绘制标题、类别标签以及图例。

(set-backend! "skia")
(canvas 400 300 :bg "#FFFFFF")

; 标题（当前默认字体对 CJK 字形支持有限，示例内容使用英文以保证显示效果）
(add (text 60 50 "PML Graphics Demo"
           :fill "#212529"
           :font-size 28))

; 类别标签
(add (text 60 120 "Category A" :fill "#495057" :font-size 16))
(add (text 60 160 "Category B" :fill "#495057" :font-size 16))
(add (text 60 200 "Category C" :fill "#495057" :font-size 16))

; 图例：色块 + 文字
(add (rect 170 105 20 20 :fill "#FF6B6B"))
(add (text 200 120 "Red Series" :fill "#212529" :font-size 16))

(add (rect 170 145 20 20 :fill "#51CF66"))
(add (text 200 160 "Green Series" :fill "#212529" :font-size 16))

(add (rect 170 185 20 20 :fill "#339AF0"))
(add (text 200 200 "Blue Series" :fill "#212529" :font-size 16))

(render "03_text_and_labels.png")
