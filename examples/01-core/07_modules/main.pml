; main.pml：导入 shapes.pml 模块并渲染结果

; 导入同一目录下的 shapes.pml（路径相对于当前文件目录解析）
(import "shapes.pml" as shapes)

(canvas 400 300 :bg "#F8F9FA")

; 使用模块提供的 circle-row 绘制一行圆
(for-each add (shapes/circle-row 80 5 20 70 "#E74C3C"))

; 使用模块提供的 rect-grid 绘制网格
(for-each add (shapes/rect-grid 60 150 4 2 35 10 "#3498DB"))

(add (text 200 30 "模块示例" :fill "#2C3E50" :font-size 20))

(render "main.png")
