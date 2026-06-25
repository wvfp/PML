;; test_triangulation.pml — Verify poly2tri triangulation works
;; Run with: pml.exe examples/test_triangulation.pml

(define (make-triangle)
  (polygon '((0 0) (100 0) (50 100)) :fill "#ff0000"))

(define (make-quad)
  (rect 0 0 100 100 :fill "#00ff00"))

(define (make-pentagon)
  (let ((pts '()))
    (for i 0 5
      (let ((a (* 2 3.14159 (/ i 5))))
        (set! pts (cons (list (+ 50 (* 50 (cos a)))
                              (+ 50 (* 50 (sin a))))
                      pts))))
    (polygon pts :fill "#0000ff"))))

;; Render shapes (triangulation happens internally)
(canvas 200 200)
(add (make-triangle))
(add (make-quad))
(add (make-pentagon))
(render "test_triangulation.png")
(print "Triangulation test: rendered test_triangulation.png")
