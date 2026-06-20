; Generate simple placeholder assets for the examples.
; Run with: pml examples/assets/gen.pml -o examples/assets

(canvas 64 64 :bg "#4CAF50")
(add (rect 0 0 64 64 :fill "#4CAF50"))
(add (rect 4 4 8 8 :fill "#45A049"))
(add (rect 20 24 8 8 :fill "#45A049"))
(add (rect 48 12 8 8 :fill "#45A049"))
(add (rect 36 44 8 8 :fill "#45A049"))
(render "grass.png" :format "png")

(canvas 64 64 :bg "transparent")
(add (rect 24 40 16 24 :fill "#8B4513"))
(add (circle 32 32 22 :fill "#228B22"))
(add (circle 22 28 12 :fill "#32CD32"))
(add (circle 42 26 14 :fill "#32CD32"))
(add (circle 32 18 12 :fill "#32CD32"))
(render "tree.png" :format "png")

(canvas 32 32 :bg "transparent")
(add (circle 16 16 12 :fill "#FFD700"))
(add (rect 14 6 4 8 :fill "#FFA500"))
(render "coin.png" :format "png")

(canvas 64 64 :bg "transparent")
(add (circle 32 20 16 :fill "#FFCCAA"))
(add (rect 20 36 24 20 :fill "#4169E1"))
(add (rect 18 34 10 8 :fill "#4169E1"))
(add (rect 36 34 10 8 :fill "#4169E1"))
(add (circle 26 18 3 :fill "#000000"))
(add (circle 38 18 3 :fill "#000000"))
(add (rect 28 24 8 4 :fill "#FF6B6B"))
(render "character.png" :format "png")

(canvas 64 64 :bg "transparent")
(add (rect 8 8 48 48 :fill "#FF6B6B" :stroke "#CC5555" :stroke-width 4))
(add (rect 16 16 12 12 :fill "#FFFFFF"))
(add (rect 36 16 12 12 :fill "#FFFFFF"))
(add (rect 16 36 32 12 :fill "#FFFFFF"))
(render "crate.png" :format "png")

; A tiny 2-frame walk-cycle spritesheet (128x64, two 64x64 frames)
(canvas 128 64 :bg "transparent")
; frame 0
(add (circle 32 20 16 :fill "#FFCCAA"))
(add (rect 20 36 24 20 :fill "#4169E1"))
(add (rect 14 34 10 8 :fill "#4169E1"))
(add (rect 40 34 10 8 :fill "#4169E1"))
(add (circle 26 18 3 :fill "#000000"))
(add (circle 38 18 3 :fill "#000000"))
; frame 1
(add (circle 96 20 16 :fill "#FFCCAA"))
(add (rect 84 36 24 20 :fill "#4169E1"))
(add (rect 78 34 10 8 :fill "#4169E1"))
(add (rect 104 34 10 8 :fill "#4169E1"))
(add (circle 90 18 3 :fill "#000000"))
(add (circle 102 18 3 :fill "#000000"))
(render "walk_sheet.png" :format "png")

; Return list of generated files
(list "grass.png" "tree.png" "coin.png" "character.png" "crate.png" "walk_sheet.png")
