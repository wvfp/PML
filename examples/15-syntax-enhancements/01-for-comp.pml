;; ============================================================
;; 01-for-comp.pml — for-comp 嵌套迭代推导式
;;
;; Syntax: (for-comp (var in <list>) body...)
;;         (for-comp (var in start end) body...)
;;         (for-comp (x in ...) (y in ...) body...)  (nested)
;; Returns a vector of body results (one per iteration).
;; ============================================================
(set-backend! "skia")
(canvas 350 350 :bg "#1a1a2e")

;; 5x5 grid using for-comp over lists
(for-comp (x in (list 35 105 175 245 315))
  (for-comp (y in (list 35 105 175 245 315))
    (add (circle x y 12 :fill "#e94560" :opacity 0.8))))

;; Diagonal highlight using for-comp range (start inclusive, end exclusive)
(for-comp (i in 0 5)
  (let ((pos (+ 35 (* i 70))))
    (add (circle pos pos 18 :fill "#fdcb6e" :stroke "#fff" :stroke-width 2))))

(render "01-for-comp.png")
