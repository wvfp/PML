;; PML Standard Library — threading.pml
;; Threading utilities
;;
;; NOTE: The -> and ->> threading is implemented as C++ builtins
;; (thread-first and thread-last). This file re-exports them.
;;
;; Usage:
;;   (thread-first 10 (+ 2) (* 3))    → 36
;;   (thread-last (list 3 1 2) (sort <) (car)) → 1

(provide)
