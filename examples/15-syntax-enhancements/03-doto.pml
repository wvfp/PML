;; ============================================================
;; 03-doto.pml — doto macro for chaining operations
;;
;; Syntax: (doto val forms...)
;; Evaluates val, then threads it through each form as the
;; first argument, discarding intermediate results and
;; returning the original value.
;; ============================================================
(set-backend! "skia")
(canvas 500 300 :bg "#1e272e")

;; Build a polyline path string with doto — each form
;; appends a segment, then the full string is returned.
(define path-str (doto "M 50 200"
                   (string-append " L 150 50")
                   (string-append " L 250 180")
                   (string-append " L 350 60")
                   (string-append " L 450 200")))

(add (path :d path-str
           :fill "none"
           :stroke "#0be881"
           :stroke-width 3))

;; Vertex dots
(for-each (lambda (xy)
            (add (circle (car xy) (cadr xy) 7 :fill "#ffdd59")))
          (list (list 50 200) (list 150 50)
                (list 250 180) (list 350 60)
                (list 450 200)))

(println "path built with doto:" path-str)

(render "03-doto.png")
