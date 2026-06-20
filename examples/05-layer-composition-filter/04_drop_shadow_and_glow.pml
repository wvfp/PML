; 示例 4：阴影 / 发光滤镜
; 展示 drop-shadow、inner-shadow、outer-glow、inner-glow。

(set-backend! "skia")

(define comp (make-composition "shadow-glow" 500 350 :bg "#F5F5F5"))

(define bg (make-layer "bg" (rect 0 0 500 350 :fill "#F5F5F5")))

; 投影
(define box-shadow (make-layer "box-shadow"
                               (rect 0 0 120 120 :fill "#3498DB" :rx 8)
                               :offset '(70 80)
                               :filter (drop-shadow :dx 8 :dy 8 :blur 8 :color "#00000060")))

; 内阴影
(define box-inner (make-layer "box-inner"
                              (rect 0 0 120 120 :fill "#E74C3C" :rx 8)
                              :offset '(230 80)
                              :filter (inner-shadow :dx 6 :dy 6 :blur 6 :color "#00000060")))

; 外发光
(define box-glow (make-layer "box-glow"
                             (circle 0 0 50 :fill "#2ECC71")
                             :offset '(120 230)
                             :filter (outer-glow :blur 16 :color "#2ECC7180")))

; 内发光（如后端不支持则 gracefully 退化）
(define box-inner-glow (make-layer "box-inner-glow"
                                   (circle 0 0 50 :fill "#9B59B6")
                                   :offset '(300 230)
                                   :filter (inner-glow :blur 12 :color "#FFFFFF80")))

(set! comp (composition-add comp bg box-shadow box-inner box-glow box-inner-glow))
(composition-render comp "04_drop_shadow_and_glow.png")
