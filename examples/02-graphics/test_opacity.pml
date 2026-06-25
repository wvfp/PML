(set-backend! "skia")
(canvas 400 300 :bg "#F5F0EB")

(define (label x y msg)
  (add (text x y msg :fill "#666" :font-size 12 :font-family "Arial")))

; Row 1: Opacity comparison
(add (rect 30 30 100 100 :fill "#FF6B6B" :stroke "#C0392B" :stroke-width 2))
(label 50 145 "opacity=1.0")

(add (rect 160 30 100 100 :fill "#FF6B6B" :stroke "#C0392B" :stroke-width 2 :opacity 0.7))
(label 175 145 "opacity=0.7")

(add (rect 290 30 100 100 :fill "#FF6B6B" :stroke "#C0392B" :stroke-width 2 :opacity 0.3))
(label 310 145 "opacity=0.3")

; Row 2: Opacity + blend mode
(add (rect 30 170 100 100 :fill "#3498DB" :blend-mode 'multiply))
(add (rect 60 200 100 100 :fill "#E74C3C" :opacity 0.5 :blend-mode 'multiply))
(label 50 285 "multiply blend")

(add (rect 180 170 100 100 :fill "#2ECC71" :blend-mode 'screen))
(add (rect 210 200 100 100 :fill "#9B59B6" :opacity 0.5 :blend-mode 'screen))
(label 200 285 "screen blend")

(add (rect 330 170 100 100 :fill "#F39C12" :opacity 0.5))

(render "examples/02-graphics/test_opacity.png")
