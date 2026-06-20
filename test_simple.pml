(set-backend! "skia")
(canvas 400 400 :bg "#FFFFFF")
(add (circle 200 200 100 :fill "#E63A3A" :stroke "#000000" :stroke-width 4))
(render "test_simple.png")
