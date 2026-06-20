; background.pml - 黑色背景上的白色圆角方块

(provide white-card)

(define (white-card)
  (rect 80 80 640 640
        :fill "#F0F0F0"
        :stroke "#1A1A1A"
        :stroke-width 10
        :rx 80))
