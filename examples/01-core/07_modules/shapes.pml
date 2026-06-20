; shapes.pml：提供 circle-row 和 rect-grid 函数模块

(define (circle-row y count radius spacing fill-color)
  (map (lambda (i)
         (circle (+ 50 (* i spacing)) y radius
                 :fill fill-color
                 :stroke "#2C3E50"
                 :stroke-width 1))
       (range 0 count)))

(define (rect-grid x0 y0 cols rows size spacing fill-color)
  (map (lambda (i)
         (define col (modulo i cols))
         (define row (quotient i cols))
         (rect (+ x0 (* col (+ size spacing)))
               (+ y0 (* row (+ size spacing)))
               size size
               :fill fill-color
               :stroke "#2C3E50"
               :stroke-width 1))
       (range 0 (* cols rows))))

(provide circle-row rect-grid)
