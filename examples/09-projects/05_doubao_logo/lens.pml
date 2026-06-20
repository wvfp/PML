; lens.pml - 中央镜头：深色外圈 + 白色内圈 + 橙色小点
; 中心点 (430, 410)

(provide camera-lens)

(define (camera-lens)
  (group
    ; 外层深色圆
    (circle 430 410 88 :fill "#4A4A6A" :stroke "#1A1A1A" :stroke-width 10)
    ; 内部白色圆
    (circle 430 410 62 :fill "#F0F0F0" :stroke "#1A1A1A" :stroke-width 8)
    ; 右上方橙色小点
    (circle 505 340 10 :fill "#FF9F1C" :stroke "#1A1A1A" :stroke-width 5)))
