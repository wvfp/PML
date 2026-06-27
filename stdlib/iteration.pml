;; PML Standard Library — iteration.pml
;;
;; NOTE: `dotimes` and `for` are implemented as C++ special forms
;; (not macros) to avoid PML's macro hygiene issue with quasiquote/unquote.
;; They are registered in `evaluator.cpp` and available without importing.
;;
;; Usage:
;;   (dotimes (var count-expr) body...)
;;     → Iterates from 0 to count-expr-1, binding var to each integer.
;;
;;   (for (var start end [step]) body...)
;;     → Iterates var from start (inclusive) to end (exclusive) by step.
;;       step defaults to 1; can be negative for descending loops.
