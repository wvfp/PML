;; portrait — character portrait sprite template
;;
;; (use-template "portrait" :expression "happy" :style "cel" :bg "transparent")

(define (portrait-main :key (expression "happy") (style "cel") (bg "transparent") (name "character"))
  (sprite-canvas 128 256 :bg bg :anchor 'center-bottom)
  (add (character :expression expression :style style))
  (render (string-append name ".png")))
