;; PML Standard Library — iteration.pml
;; Iteration macros (for loop, etc.)
;;
;; IMPORTANT: Avoid let/lambda bindings inside macro bodies — PML's macro
;; hygiene renames them, but references inside quasiquote unquote forms
;; are not correspondingly renamed.

;; Numeric for loop: (for (var start end [step]) body...)
;; step defaults to 1. Loop includes start, excludes end.
(defmacro for (bindings . body)
  (let ((var (car bindings))
        (start (cadr bindings))
        (_end (caddr bindings))
        (_step (if (null? (cdddr bindings)) 1 (cadddr bindings))))
    `(letrec ((loop (lambda (,var)
                      (when (<= ,var ,_end)
                        ,@body
                        (loop (+ ,var ,_step))))))
       (loop ,start))))

(provide for)
