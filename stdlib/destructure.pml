;; PML Standard Library — destructure.pml
;; Destructuring bind macro (flat list destructuring)
;;
;; IMPORTANT: Avoid let/lambda bindings inside macro bodies — PML's macro
;; hygiene renames them, but references inside quasiquote unquote forms
;; are not correspondingly renamed.

;; Flat list destructuring: (let-dest ((a b c) list-expr) body...)
;; Expands to: (let ((a (list-ref list-expr 0)) (b (list-ref list-expr 1)) ...) body...)
(defmacro let-dest (bindings . body)
  (let ((names (car bindings))
        (val-expr (cadr bindings)))
    (letrec ((mk-bindings
              (lambda (ns i)
                (if (null? ns)
                    '()
                    (cons (list (car ns) (list 'list-ref val-expr i))
                          (mk-bindings (cdr ns) (+ i 1)))))))
      `(let ,(mk-bindings names 0) ,@body))))

(provide let-dest)
