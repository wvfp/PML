; hand.pml - 比耶的 V 字手势（食指和中指伸出，其他手指握拳）
; 手位于画面左侧 (200, 700)

(import "palette.pml" as p)

(provide hand-main)

; 手掌（握拳的主体）
(define (palm)
  (polygon (list
             (list 80 680)
             (list 60 720)
             (list 55 770)
             (list 70 820)
             (list 110 840)
             (list 160 845)
             (list 200 835)
             (list 215 800)
             (list 210 760)
             (list 190 710)
             (list 150 685))
           :fill p/c-skin
           :stroke p/c-stroke
           :stroke-width 8))

; 食指（最右的伸出指）
(define (finger-index)
  (group
    (ellipse 160 600 28 70
             :fill p/c-skin
             :stroke p/c-stroke
             :stroke-width 8)
    (line 130 580 180 575
          :stroke p/c-skin-shadow
          :stroke-width 3)
    (line 125 620 175 615
          :stroke p/c-skin-shadow
          :stroke-width 3)))

; 中指（食指左边的伸出指）
(define (finger-middle)
  (group
    (ellipse 105 600 28 70
             :fill p/c-skin
             :stroke p/c-stroke
             :stroke-width 8)
    (line 75 580 125 575
          :stroke p/c-skin-shadow
          :stroke-width 3)
    (line 70 620 120 615
          :stroke p/c-skin-shadow
          :stroke-width 3)))

; 无名指和小指（握拳，弯曲藏在手心）
(define (fingers-folded)
  (group
    (ellipse 150 700 35 28
             :fill p/c-skin
             :stroke p/c-stroke
             :stroke-width 8)
    (ellipse 130 740 35 28
             :fill p/c-skin
             :stroke p/c-stroke
             :stroke-width 8)))

; 拇指
(define (thumb)
  (polygon (list
             (list 190 705)
             (list 220 725)
             (list 230 760)
             (list 220 795)
             (list 200 805)
             (list 195 780)
             (list 205 745)
             (list 190 720))
           :fill p/c-skin
           :stroke p/c-stroke
           :stroke-width 8))

; 手背上的细节阴影
(define (hand-marks)
  (group
    (line 135 670 155 680
          :stroke p/c-skin-shadow
          :stroke-width 3)
    (polygon (list
               (list 90 730)
               (list 110 750)
               (list 130 735)
               (list 135 755)
               (list 100 775)
               (list 85 760))
             :fill p/c-skin-shadow
             :stroke p/c-stroke
             :stroke-width 2)))

; 袖口（红色毛衣袖口）
(define (cuff)
  (group
    (polygon (list
               (list 30 790)
               (list 40 860)
               (list 60 900)
               (list 100 920)
               (list 160 915)
               (list 210 900)
               (list 230 870)
               (list 230 820)
               (list 205 800)
               (list 160 800)
               (list 120 795)
               (list 80 795))
             :fill p/c-clothes
             :stroke p/c-stroke
             :stroke-width 8)
    (polygon (list
               (list 30 790)
               (list 50 810)
               (list 110 818)
               (list 170 818)
               (list 215 810)
               (list 230 820)
               (list 220 835)
               (list 170 845)
               (list 110 845)
               (list 50 838)
               (list 30 820))
             :fill p/c-clothes-shadow
             :stroke p/c-stroke
             :stroke-width 6)
    (line 70 810 70 840 :stroke p/c-clothes-rib :stroke-width 3)
    (line 100 810 100 840 :stroke p/c-clothes-rib :stroke-width 3)
    (line 130 810 130 840 :stroke p/c-clothes-rib :stroke-width 3)
    (line 160 810 160 840 :stroke p/c-clothes-rib :stroke-width 3)
    (line 190 810 190 840 :stroke p/c-clothes-rib :stroke-width 3)))

; 整个手 + 袖口
(define (hand-main)
  (group
    (cuff)
    (palm)
    (fingers-folded)
    (thumb)
    (finger-middle)
    (finger-index)
    (hand-marks)))
