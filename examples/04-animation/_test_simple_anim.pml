; test simple animation
(set-backend! "skia")
(canvas 200 100 :bg "#FFFFFF")

(define box (rect 10 40 30 30 :fill "#FF0000"))
(add box)

(define anim (animate box 'x 10 150 2.0 :ease 'linear))
(play)

(render "_test_anim.gif" :fps 10)
(print "done")
