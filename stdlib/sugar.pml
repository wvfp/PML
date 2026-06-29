;; PML Standard Library — sugar.pml
;; Syntactic sugar: defn, defconst
;;
;; Usage: (import "sugar.pml" as sugar)
;;        (sugar/defn greet (name) (string-append "Hello, " name))
;;        (sugar/defconst PI 3.14159)

;; define shorthand for functions
(defmacro defn (name params . body)
  `(define (,name ,@params) ,@body))

;; define shorthand for constants
(defmacro defconst (name value)
  `(define ,name ,value))

;; doto — chain side-effectful calls returning the value
;;   (doto (list 1 2 3) (push! 4) (push! 5))
;; →
;;   (let ((g (list 1 2 3))) (push! g 4) (push! g 5) g)
(defmacro doto (val . forms)
  (let ((g (gensym)))
    `(let ((,g ,val))
       ,@(map (lambda (f)
               (if (pair? f)
                   `(,(car f) ,g ,@(cdr f))
                   (list f g)))
             forms)
       ,g)))

(provide defn defconst doto)
