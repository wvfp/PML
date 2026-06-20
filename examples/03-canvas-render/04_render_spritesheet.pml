; 示例 4：使用 render-spritesheet 生成精灵表
; 运行：pml.exe examples/03-canvas-render/04_render_spritesheet.pml -o examples/output
; 输出：04_render_spritesheet.png（同时生成 04_render_spritesheet.spritesheet.json）

; 定义 4 个不同样式的精灵
(define sprite-red
  (circle 32 32 24
          :fill "#EF5350"
          :stroke "#B71C1C"
          :stroke-width 2))

(define sprite-green
  (rect 8 8 48 48
        :fill "#66BB6A"
        :stroke "#1B5E20"
        :stroke-width 2
        :rx 6))

(define sprite-blue
  (ellipse 32 32 26 18
           :fill "#42A5F5"
           :stroke "#0D47A1"
           :stroke-width 2))

(define sprite-gold
  (polygon (list (list 32 10) (list 50 28) (list 42 54) (list 22 54) (list 14 28))
           :fill "#FFCA28"
           :stroke "#F57F17"
           :stroke-width 2))

; 将 4 个精灵排列成 2x2 的精灵表，每个单元 64x64，间隔 4 像素
(render-spritesheet "04_render_spritesheet.png"
                    sprite-red
                    sprite-green
                    sprite-blue
                    sprite-gold
                    :cols 2
                    :cell-width 64
                    :cell-height 64
                    :padding 4
                    :bg "#EEEEEE")
