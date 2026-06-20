; 示例 2：图层组
; 用 make-group 把多个 layer 编组，整体位移/变换。

(set-backend! "skia")

(define comp (make-composition "layer-groups" 500 350 :bg "#2C3E50"))

; 单个图元
(define eye-white (make-layer "eye-white"
                              (ellipse 0 0 40 50 :fill "#FFFFFF")
                              :offset '(0 0)))
(define iris (make-layer "iris"
                         (circle 0 0 18 :fill "#3498DB")
                         :offset '(0 0)))
(define pupil (make-layer "pupil"
                          (circle 0 0 8 :fill "#000000")
                          :offset '(0 0)))

; 把眼睛部件编组，作为一个整体复用
(define eye (make-group "eye" eye-white iris pupil))

; 左眼组：整体位移并放大
(define left-eye (layer-with eye :offset '(150 175) :transform (scale 1.2 1.2)))

; 右眼组：整体位移、放大并水平翻转
(define right-eye (layer-with eye :offset '(350 175) :transform (compose (scale 1.2 1.2) (scale -1.0 1.0))))

(set! comp (composition-add comp left-eye right-eye))
(composition-render comp "02_layer_groups.png")
