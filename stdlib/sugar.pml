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

(provide defn defconst)
