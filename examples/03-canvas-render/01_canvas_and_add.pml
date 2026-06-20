; 示例 1：创建画布并添加多个形状
; 运行：pml.exe examples/03-canvas-render/01_canvas_and_add.pml -o examples/output
; 输出：01_canvas_and_add.png

; 创建 400x400 的白色画布
(canvas 400 400 :bg "#FFFFFF")

; 红色填充、蓝色描边的圆
(define sun
  (circle 320 80 40
          :fill "#F1C40F"
          :stroke "#E67E22"
          :stroke-width 3))

; 绿色矩形作为草地
(define ground
  (rect 0 300 400 100
        :fill "#2ECC71"
        :stroke "#27AE60"
        :stroke-width 2))

; 蓝色椭圆作为池塘
(define pond
  (ellipse 100 250 60 35
           :fill "#3498DB"
           :stroke "#2980B9"
           :stroke-width 2))

; 棕色三角形作为山峰
(define mountain
  (polygon (list (list 50 300) (list 150 150) (list 250 300))
           :fill "#795548"
           :stroke "#5D4037"
           :stroke-width 2))

; 连接山顶与太阳的线
(define ray
  (line 150 150 290 110
        :stroke "#E74C3C"
        :stroke-width 2))

; 将所有图形添加到当前画布
(add sun)
(add ground)
(add pond)
(add mountain)
(add ray)

; 渲染并保存为 PNG
(render "01_canvas_and_add.png")
