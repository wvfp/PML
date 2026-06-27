;; PML Standard Library — iteration.pml
;;
;; NOTE: `dotimes` is implemented as a C++ special form (not a macro) to
;; avoid PML's macro hygiene issue with quasiquote/unquote. It is registered
;; in `evaluator.cpp` and available without importing this file.
;;
;; Usage: (dotimes (var count-expr) body...)
;;   → Iterates from 0 to count-expr-1, binding var to each integer.
