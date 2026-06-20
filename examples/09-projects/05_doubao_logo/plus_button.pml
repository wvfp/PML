; plus_button.pml - 蓝色加号按钮

(provide plus-button)

(define (plus-button)
  (group
    ; 外圆
    (circle 200 620 42 :fill "#1FA8E0" :stroke "#1A1A1A" :stroke-width 7)
    ; 白色横线
    (rect 176 613 48 14 :fill "#FFFFFF")
    ; 白色竖线
    (rect 193 596 14 48 :fill "#FFFFFF")))
