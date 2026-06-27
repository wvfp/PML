;; Test: name/lookup/set-property (F9)
(canvas 200 200 :bg "#FFFFFF")

;; Create a named rectangle, add it, and modify via set-property
(define my-rect (name "box-1" (rect 50 50 100 100 :fill "#FF0000")))
(add my-rect)

;; Modify the named object's properties
(add (set-property "box-1" :fill "#00FF00" :opacity 0.7))

;; Test that remove works and lookup returns nil after removal
(remove "box-1")

;; If lookup returns something (shouldn't), draw a blue square
(define after (lookup "box-1"))
(if after
  (add (rect 0 0 10 10 :fill "#0000FF"))
  ;; else draw a dim bar at the bottom to confirm fallback
  (add (rect 10 180 180 10 :fill "#000000" :opacity 0.3)))

(render "name_test.png")
