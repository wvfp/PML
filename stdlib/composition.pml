;; PML Standard Library — composition.pml
;; Layer composition macros
;;
;; IMPORTANT: Avoid let/lambda bindings inside macro bodies — PML's macro
;; hygiene renames them, but references inside quasiquote unquote forms
;; are not correspondingly renamed.

;; Create a composition with layers in a single expression.
(defmacro with-composition (args . body)
  (if (and (pair? (car (reverse body)))
           (equal? (car (car (reverse body))) 'render))
      (let ((comp-name (car args))
            (comp-w (cadr args))
            (comp-h (caddr args))
            (comp-kwargs (if (> (length args) 3) (cdddr args) '()))
            (render-path (cadr (car (reverse body))))
            (layers (reverse (cdr (reverse body)))))
        `(let ((_c (make-composition ,comp-name ,comp-w ,comp-h ,@comp-kwargs)))
           ,@(map (lambda (b)
                    (if (and (pair? b) (equal? (car b) 'layer))
                        `(set! _c (composition-add _c (make-layer ,@(cdr b))))
                        `(set! _c (composition-add _c ,b))))
                  layers)
           (composition-render _c ,render-path)))
      (let ((comp-name (car args))
            (comp-w (cadr args))
            (comp-h (caddr args))
            (comp-kwargs (if (> (length args) 3) (cdddr args) '())))
        `(let ((_c (make-composition ,comp-name ,comp-w ,comp-h ,@comp-kwargs)))
           ,@(map (lambda (b)
                    (if (and (pair? b) (equal? (car b) 'layer))
                        `(set! _c (composition-add _c (make-layer ,@(cdr b))))
                        `(set! _c (composition-add _c ,b))))
                  body)
           _c))))

(provide with-composition)
