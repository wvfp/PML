; 示例 3：使用 render-set 导出多尺寸图标
; 运行：pml.exe examples/03-canvas-render/03_render_set.pml -o examples/output
; 输出：render_set_icon@1x.png、render_set_icon@2x.png、render_set_icon@4x.png

; 定义一个 64x64 的宝石图标作为基础内容
(define gem-icon
  (group
    ; 外框菱形
    (polygon (list (list 32 4) (list 58 32) (list 32 60) (list 6 32))
             :fill "#9C27B0"
             :stroke "#4A148C"
             :stroke-width 2)
    ; 内部高光
    (polygon (list (list 32 12) (list 48 32) (list 32 52) (list 16 32))
             :fill "#E1BEE7")
    ; 中心闪光点
    (circle 32 32 5 :fill "#FFFFFF")))

; 导出 1x/2x/4x 三种尺寸的图标集
(render-set "render_set_icon"
            :content gem-icon
            :scales (list 1 2 4)
            :base-size (list 64 64)
            :format "PNG")
