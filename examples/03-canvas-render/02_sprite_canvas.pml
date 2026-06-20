; 示例 2：精灵画布（透明背景）
; 运行：pml.exe examples/03-canvas-render/02_sprite_canvas.pml -o examples/output
; 输出：02_sprite_canvas.png（同时会生成 02_sprite_canvas.meta.json）

; 创建 200x200 的精灵画布，背景透明，锚点默认在底部中心
(sprite-canvas 200 200 :bg "transparent")

; 用简单的几何图形拼出一个小人
(define head
  (circle 0 -80 25
          :fill "#FFCC80"
          :stroke "#E65100"
          :stroke-width 2))

(define body
  (rect -30 -50 60 70
        :fill "#42A5F5"
        :stroke "#1565C0"
        :stroke-width 2
        :rx 8))

(define left-eye
  (circle -8 -85 3 :fill "#212121"))

(define right-eye
  (circle 8 -85 3 :fill "#212121"))

(define smile
  (line -10 -72 10 -72
        :stroke "#E65100"
        :stroke-width 2))

; 将各部位组合成一个整体
(define character (group head body left-eye right-eye smile))

; 添加到精灵画布（会自动按精灵锚点居中）
(add character)

; 渲染并保存为 PNG，透明背景
(render "02_sprite_canvas.png")
