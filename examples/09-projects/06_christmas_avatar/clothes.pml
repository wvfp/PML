; clothes.pml - 红色毛衣（上半身衣服）
; 衣服覆盖颈部下方到画面底部

(import "palette.pml" as p)

(provide clothes-main)

; 毛衣主体（大梯形轮廓，覆盖颈部下方的上半身）
(define (clothes-body)
  (polygon (list
             (list 360 830)
             (list 320 870)
             (list 250 900)
             (list 180 920)
             (list 80 960)
             (list 30 1000)
             (list 0 1000)
             (list 0 870)
             (list 100 820)
             (list 160 810)
             (list 200 790)
             (list 220 780)
             (list 640 830)
             (list 680 870)
             (list 750 900)
             (list 820 920)
             (list 920 960)
             (list 970 1000)
             (list 1000 1000)
             (list 1000 870)
             (list 900 820)
             (list 840 810)
             (list 800 790)
             (list 780 780))
           :fill p/c-clothes
           :stroke p/c-stroke
           :stroke-width 8))

; 领口圆环（U形）
(define (neck-collar)
  (polygon (list
             (list 360 830)
             (list 370 850)
             (list 400 880)
             (list 440 895)
             (list 500 900)
             (list 560 895)
             (list 600 880)
             (list 630 850)
             (list 640 830)
             (list 625 825)
             (list 615 850)
             (list 580 870)
             (list 540 875)
             (list 500 875)
             (list 460 875)
             (list 420 870)
             (list 385 850)
             (list 375 825))
           :fill p/c-clothes-shadow
           :stroke p/c-stroke
           :stroke-width 6))

; 毛衣褶皱/阴影线条
(define (clothes-shadows)
  (group
    (polygon (list
               (list 250 910)
               (list 230 970)
               (list 260 1000)
               (list 320 1000)
               (list 340 950))
             :fill p/c-clothes-shadow
             :stroke p/c-stroke
             :stroke-width 3)
    (polygon (list
               (list 750 910)
               (list 770 970)
               (list 740 1000)
               (list 680 1000)
               (list 660 950))
             :fill p/c-clothes-shadow
             :stroke p/c-stroke
             :stroke-width 3)
    (polygon (list
               (list 480 910)
               (list 495 950)
               (list 500 1000)
               (list 520 1000)
               (list 515 950)
               (list 530 910))
             :fill p/c-clothes-shadow
             :stroke p/c-stroke
             :stroke-width 3)))

; 领口罗纹装饰线
(define (collar-ribs)
  (group
    (line 400 865 400 890 :stroke p/c-clothes-rib :stroke-width 3)
    (line 430 875 430 898 :stroke p/c-clothes-rib :stroke-width 3)
    (line 470 880 470 900 :stroke p/c-clothes-rib :stroke-width 3)
    (line 500 880 500 900 :stroke p/c-clothes-rib :stroke-width 3)
    (line 530 880 530 900 :stroke p/c-clothes-rib :stroke-width 3)
    (line 570 875 570 898 :stroke p/c-clothes-rib :stroke-width 3)
    (line 600 865 600 890 :stroke p/c-clothes-rib :stroke-width 3)))

; 整个衣服
(define (clothes-main)
  (group
    (clothes-body)
    (clothes-shadows)
    (neck-collar)
    (collar-ribs)))
