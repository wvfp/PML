; arc.pml - 镜头右侧的彩色 C 形齿轮
; 中心点 (430, 410)，与镜头同心

(provide color-wheel)

(define (point-on-circle cx cy r angle)
  (list (+ cx (* r (cos angle)))
        (+ cy (* r (sin angle)))))

(define (gen-outer-points cx cy r a0 a1 steps n pts)
  (if (> n steps)
      pts
      (gen-outer-points cx cy r a0 a1 steps (+ n 1)
                        (append pts (list (point-on-circle cx cy r
                                       (+ a0 (* (- a1 a0) (/ n steps)))))))))

(define (gen-inner-points cx cy r a0 a1 steps n pts)
  (if (< n 0)
      pts
      (gen-inner-points cx cy r a0 a1 steps (- n 1)
                        (append pts (list (point-on-circle cx cy r
                                       (+ a0 (* (- a1 a0) (/ n steps)))))))))

(define (arc-segment cx cy r-outer r-inner a0 a1 color)
  (polygon (append (gen-outer-points cx cy r-outer a0 a1 18 0 '())
                   (gen-inner-points cx cy r-inner a0 a1 18 18 '()))
           :fill color
           :stroke "#1A1A1A"
           :stroke-width 8))

(define (gear-tooth cx cy r-outer a h w)
  (polygon (list (point-on-circle cx cy (+ r-outer h) a)
                 (point-on-circle cx cy r-outer (- a w))
                 (point-on-circle cx cy r-outer (+ a w)))
           :fill "#1A1A1A"
           :stroke "#1A1A1A"
           :stroke-width 1))

(define (deg->rad d) (* d (/ 3.14159265 180.0)))

(define (concat a b)
  (if (null? a) b (concat (cdr a) (append b (list (car a))))))

(define (color-wheel)
  (let* ((cx 430) (cy 410) (r-in 95) (r-out 190)
         ; C 形开口在右侧：弧从左上 (-130°) 经过上/右/下 到左下 (+130°)
         ; 屏幕坐标：0°=右, -90°=上, 90°=下
         (a-start (deg->rad -130.0))
         (a-end   (deg->rad  130.0))
         (segments
           (list
             (arc-segment cx cy r-out r-in a-start   (deg->rad -100.0) "#FF2E93")
             (arc-segment cx cy r-out r-in (deg->rad -100.0) (deg->rad -70.0) "#FF3D6E")
             (arc-segment cx cy r-out r-in (deg->rad -70.0) (deg->rad -40.0) "#FF4D4A")
             (arc-segment cx cy r-out r-in (deg->rad -40.0) (deg->rad -10.0) "#FF6A3D")
             (arc-segment cx cy r-out r-in (deg->rad -10.0) (deg->rad  20.0) "#FF9533")
             (arc-segment cx cy r-out r-in (deg->rad  20.0) (deg->rad  50.0) "#FFB92E")
             (arc-segment cx cy r-out r-in (deg->rad  50.0) (deg->rad  80.0) "#FFD93D")
             (arc-segment cx cy r-out r-in (deg->rad  80.0) (deg->rad 110.0) "#FFE066")
             (arc-segment cx cy r-out r-in (deg->rad 110.0) a-end            "#FFF0A0")))
         (teeth
           (list
             (gear-tooth cx cy r-out (deg->rad -115.0) 18 0.11)
             (gear-tooth cx cy r-out (deg->rad  -85.0) 18 0.11)
             (gear-tooth cx cy r-out (deg->rad  -55.0) 18 0.11)
             (gear-tooth cx cy r-out (deg->rad  -25.0) 18 0.11)
             (gear-tooth cx cy r-out (deg->rad    5.0) 18 0.11)
             (gear-tooth cx cy r-out (deg->rad   35.0) 18 0.11)
             (gear-tooth cx cy r-out (deg->rad   65.0) 18 0.11)
             (gear-tooth cx cy r-out (deg->rad   95.0) 18 0.11)
             (gear-tooth cx cy r-out (deg->rad  125.0) 18 0.11))))
    (apply group (concat segments teeth))))
