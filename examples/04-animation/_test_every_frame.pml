; test render with fps
(set-backend! "skia")
(canvas 100 100 :bg "#0B0C10")

(define frame-count 0)

(every-frame
  (lambda ()
    (set! frame-count (+ frame-count 1))
    (add (circle 50 50 20 :fill "#FF0000"))))

(print "before render")
(render "_test_fps.gif" :fps 10 :duration 1.0)
(print "after render")
(print "frame-count:")
(print frame-count)
