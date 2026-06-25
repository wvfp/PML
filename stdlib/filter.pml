;; PML Standard Library — filter.pml
;; Filter pipeline macro
;;
;; Usage: (import "filter.pml" as filter)
;;        (filter/->filter (blur 4) (drop-shadow :dx 3 :dy 3 :blur 6 :color "#00000040"))

;; Compose filters in a linear pipeline.
;; Expands to (filter-chain ...) — the builtin filter chain constructor.
(defmacro ->filter (. forms)
  (if (null? forms)
      (error "->filter: at least one filter required")
      `(filter-chain ,@forms)))

(provide ->filter)
