; body.pml — 行走循环角色的身体构建函数

(provide make-walker)

(define (make-walker x y scale palette-name)
  (translate-object
    (character :expression 'happy
               :style 'cel
               :scale scale
               :palette palette-name)
    x y))
