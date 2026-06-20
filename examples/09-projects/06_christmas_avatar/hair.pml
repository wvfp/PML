; hair.pml - 灰色短发 + 刘海
; 脸中心约在 (500, 530)，头发在脸周围并向下延伸

(import "palette.pml" as p)

(provide hair-main)

; 生成圆上点
(define (point-on-circle cx cy r deg)
  (let ((rad (* deg (/ 3.14159265 180))))
    (list (+ cx (* r (cos rad)))
          (+ cy (* r (sin rad))))))

; 生成弧点
(define (gen-arc-points cx cy r start-deg end-deg steps acc)
  (if (= steps 0)
      (reverse acc)
      (let ((step (/ (- end-deg start-deg) steps)))
        (gen-arc-points cx cy r
                         (+ start-deg step)
                         end-deg
                         (- steps 1)
                         (cons (point-on-circle cx cy r start-deg) acc)))))

; 后脑勺 / 主体头发：从帽子下延伸下来覆盖到肩膀两侧
; 轮廓：顶部沿帽子内弧 + 两侧发梢（不规则）+ 颈后发梢
(define (hair-back)
  (let* ((cx 500)
         (cy 520)
         ; 头顶弧形点（帽檐下沿）
         (top-pts (gen-arc-points cx cy 240 200 340 20 '()))
         ; 左发梢（向下向外展开）
         (left-pts (list
                     (list (- cx 240) (+ cy 60))
                     (list (- cx 260) (+ cy 120))
                     (list (- cx 280) (+ cy 180))
                     (list (- cx 270) (+ cy 240))
                     (list (- cx 250) (+ cy 300))
                     (list (- cx 220) (+ cy 340))
                     (list (- cx 180) (+ cy 360))
                     (list (- cx 150) (+ cy 350))
                     (list (- cx 170) (+ cy 320))
                     (list (- cx 190) (+ cy 280))
                     (list (- cx 180) (+ cy 230))
                     (list (- cx 170) (+ cy 180))
                     (list (- cx 150) (+ cy 140))
                     (list (- cx 130) (+ cy 100))
                     ))
         ; 右发梢
         (right-pts (list
                      (list (+ cx 130) (+ cy 100))
                      (list (+ cx 150) (+ cy 140))
                      (list (+ cx 170) (+ cy 180))
                      (list (+ cx 180) (+ cy 230))
                      (list (+ cx 190) (+ cy 280))
                      (list (+ cx 170) (+ cy 320))
                      (list (+ cx 150) (+ cy 350))
                      (list (+ cx 180) (+ cy 360))
                      (list (+ cx 220) (+ cy 340))
                      (list (+ cx 250) (+ cy 300))
                      (list (+ cx 270) (+ cy 240))
                      (list (+ cx 280) (+ cy 180))
                      (list (+ cx 260) (+ cy 120))
                      (list (+ cx 240) (+ cy 60))
                      )))
    (polygon (append left-pts top-pts right-pts)
             :fill p/c-hair
             :stroke p/c-stroke
             :stroke-width 8)))

; 头发阴影（发丝层次）：在左侧和底部加阴影
(define (hair-shadows)
  (group
    ; 左侧发梢阴影
    (polygon (list
               (list 260 600)
               (list 230 650)
               (list 220 700)
               (list 250 760)
               (list 280 800)
               (list 320 790)
               (list 300 740)
               (list 310 680)
               (list 290 640))
             :fill p/c-hair-shadow
             :stroke p/c-stroke
             :stroke-width 3)
    ; 右侧发梢阴影
    (polygon (list
               (list 740 600)
               (list 770 650)
               (list 780 700)
               (list 750 760)
               (list 720 800)
               (list 680 790)
               (list 700 740)
               (list 690 680)
               (list 710 640))
             :fill p/c-hair-shadow
             :stroke p/c-stroke
             :stroke-width 3)
    ; 后脑勺中央阴影（在脸后面）
    (polygon (list
               (list 420 620)
               (list 410 680)
               (list 400 740)
               (list 430 780)
               (list 470 770)
               (list 500 760)
               (list 530 770)
               (list 570 780)
               (list 600 740)
               (list 590 680)
               (list 580 620)
               (list 550 600)
               (list 500 590)
               (list 450 600))
             :fill p/c-hair-shadow
             :stroke p/c-stroke
             :stroke-width 3)))

; 刘海：覆盖额头的前部头发（齐刘海但不规则边缘）
; 刘海中心 (500, 390)
(define (hair-bangs)
  (group
    ; 主刘海（从帽檐下伸出，斜向切过额头）
    (polygon (list
               ; 左侧起点（帽檐下方）
               (list 280 360)
               (list 310 320)
               (list 360 300)
               (list 420 300)
               (list 470 310)
               (list 500 330)
               (list 540 310)
               (list 590 300)
               (list 640 305)
               (list 690 325)
               (list 720 365)
               ; 刘海下沿（不规则锯齿）
               (list 690 400)
               (list 660 430)
               (list 630 460)
               (list 600 445)
               (list 570 475)
               (list 540 455)
               (list 510 475)
               (list 480 455)
               (list 450 475)
               (list 420 455)
               (list 390 475)
               (list 360 455)
               (list 330 475)
               (list 300 445)
               (list 280 410))
             :fill p/c-hair
             :stroke p/c-stroke
             :stroke-width 8)
    ; 刘海高光（右上部分的青白色高光）
    (polygon (list
               (list 530 315)
               (list 560 305)
               (list 600 300)
               (list 640 305)
               (list 630 335)
               (list 590 355)
               (list 555 355)
               (list 530 340))
             :fill p/c-hair-highlight
             :stroke p/c-stroke
             :stroke-width 2)))

; 组合头发：后面 -> 阴影 -> 刘海
(define (hair-main)
  (group
    (hair-back)
    (hair-shadows)
    (hair-bangs)))