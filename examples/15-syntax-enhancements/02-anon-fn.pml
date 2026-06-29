;; ============================================================
;; 02-anon-fn.pml — #() anonymous function literal
;;
;; Syntax: #(body-expr)   → (lambda (%) body-expr)
;;         #(%1 %2 body)  → (lambda (%1 %2) body)
;; Uses % for single arg, %1..%n for positional args.
;; Useful with map, for-each, and as inline callbacks.
;; ============================================================
(set-backend! "skia")
(canvas 420 200 :bg "#2d2d2d")

;; --- Single-arg #(): blend color by opacity ---
(define with-alpha #((color/rgba 233 69 96 %)))
(define alpha-50 (with-alpha 0.5))
(define alpha-80 (with-alpha 0.8))
(println "alpha-50:" alpha-50)

;; --- #() with map: create row of dots ---
(define xs (list 40 100 160 220 280 340 400))
(define dots (map #(circle % 50 12 :fill alpha-80) xs))
(for-each #(add %) dots)

;; --- #() for paired coordinates: zip xs/ys manually ---
(define ys (list 120 140 100 160 90 130 110))
(for-each (lambda (pair)
            (add (circle (car pair) (cadr pair) 8 :fill "#fdcb6e")))
          (map (lambda (i) (list (get xs i) (get ys i)))
               (range 0 7)))

;; --- #() with filter ---
(define big-ones (filter #(> % 20) (list 12 25 8 30 15)))
(println "big ones:" big-ones)

(render "02-anon-fn.png")
