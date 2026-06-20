; hat.pml - 红色圣诞帽 + 白色毛绒帽边 + 帽耳
; 帽子中心约在画布水平中央 (500, 250)

(import "palette.pml" as p)

(provide hat-main)

; 生成圆上点的辅助函数
(define (point-on-circle cx cy r deg)
  (let ((rad (* deg (/ 3.14159265 180))))
    (list (+ cx (* r (cos rad)))
          (+ cy (* r (sin rad))))))

; 在圆上生成一组点（用于弧形）
(define (gen-arc-points cx cy r start-deg end-deg steps acc)
  (if (= steps 0)
      (reverse acc)
      (let ((step (/ (- end-deg start-deg) steps)))
        (gen-arc-points cx cy r
                         (+ start-deg step)
                         end-deg
                         (- steps 1)
                         (cons (point-on-circle cx cy r start-deg) acc)))))

; 圣诞帽主体：上半圆弧 + 波浪形底边
; 帽身轮廓点（从左上开始顺时针）：外部弧 + 内部波浪底边
(define (hat-body)
  (let* ((cx 500)
         (cy 240)
         (r-outer 270)
         (r-inner 250)
         ; 上半圆弧（外部轮廓）：从 180 度到 0 度（左边到右边，在上半圆）
         ; 在 PML 中角度：0度为右，90度为下，180度为左，270度为上
         (outer-pts (gen-arc-points cx cy r-outer 180 0 30 '()))
         ; 右侧帽耳（椭圆近似）：向右下方凸出的圆
         (right-ear (list
                      (list (+ cx 250) (+ cy 20))   ; 底部右
                      (list (+ cx 280) (+ cy 60))   ; 外耳底部
                      (list (+ cx 300) (+ cy 120))  ; 耳尖
                      (list (+ cx 280) (+ cy 160))  ; 耳下
                      (list (+ cx 240) (+ cy 180))  ; 右耳底
                      ))
         ; 左侧帽耳
         (left-ear (list
                     (list (- cx 240) (+ cy 180))  ; 左耳底
                     (list (- cx 280) (+ cy 160))  ; 耳下
                     (list (- cx 300) (+ cy 120))  ; 耳尖
                     (list (- cx 280) (+ cy 60))   ; 外耳底部
                     (list (- cx 250) (+ cy 20))   ; 底部左
                     )))
    ; 帽身外部轮廓
    (polygon (append left-ear outer-pts right-ear)
             :fill p/c-hat
             :stroke p/c-stroke
             :stroke-width 8)))

; 白色毛绒帽檐（波浪形底边）
(define (hat-fur-trim)
  (let* ((cx 500)
         (cy 240)
         (r-outer 275)
         ; 帽檐底边：一系列向下凸的半圆（波浪）
         ; 外层弧：用一个厚的波浪带
         (top-curve (gen-arc-points cx cy r-outer 180 0 30 '()))
         ; 底部波浪：用一组点模拟波浪边
         (wave-bottom (list
                        (list (+ cx 260) (+ cy 90))
                        (list (+ cx 240) (+ cy 120))
                        (list (+ cx 210) (+ cy 100))
                        (list (+ cx 180) (+ cy 125))
                        (list (+ cx 150) (+ cy 100))
                        (list (+ cx 115) (+ cy 125))
                        (list (+ cx 80) (+ cy 100))
                        (list (+ cx 45) (+ cy 125))
                        (list (+ cx 10) (+ cy 105))
                        (list (- cx 25) (+ cy 125))
                        (list (- cx 60) (+ cy 100))
                        (list (- cx 95) (+ cy 125))
                        (list (- cx 130) (+ cy 100))
                        (list (- cx 165) (+ cy 125))
                        (list (- cx 200) (+ cy 100))
                        (list (- cx 235) (+ cy 125))
                        (list (- cx 260) (+ cy 90))
                        ))
         ; 左帽耳白色填充
         (left-ear (list
                     (list (- cx 255) (+ cy 180))
                     (list (- cx 295) (+ cy 170))
                     (list (- cx 315) (+ cy 125))
                     (list (- cx 295) (+ cy 70))
                     (list (- cx 260) (+ cy 55))
                     (list (- cx 230) (+ cy 95))
                     (list (- cx 230) (+ cy 130))
                     (list (- cx 210) (+ cy 160))
                     ))
         (right-ear (list
                      (list (+ cx 255) (+ cy 180))
                      (list (+ cx 295) (+ cy 170))
                      (list (+ cx 315) (+ cy 125))
                      (list (+ cx 295) (+ cy 70))
                      (list (+ cx 260) (+ cy 55))
                      (list (+ cx 230) (+ cy 95))
                      (list (+ cx 230) (+ cy 130))
                      (list (+ cx 210) (+ cy 160))
                      )))
    (group
      ; 白色毛绒帽檐（主体部分）
      (polygon (append (reverse wave-bottom) top-curve)
               :fill p/c-hat-fur
               :stroke p/c-stroke
               :stroke-width 8)
      ; 左耳白色
      (polygon left-ear
               :fill p/c-hat-fur
               :stroke p/c-stroke
               :stroke-width 8)
      ; 右耳白色
      (polygon right-ear
               :fill p/c-hat-fur
               :stroke p/c-stroke
               :stroke-width 8))))

; 帽身阴影（帽檐内侧的阴影条）
(define (hat-shadow-band)
  (let* ((cx 500)
         (cy 240)
         (r1 260)
         (r2 245)
         (outer-pts (gen-arc-points cx cy r1 170 10 25 '()))
         (inner-pts (gen-arc-points cx cy r2 10 170 25 '())))
    (polygon (append outer-pts inner-pts)
             :fill p/c-hat-shadow
             :stroke '()
             :stroke-width 0)))

; 组合整个帽子：阴影 -> 帽身 -> 毛绒檐
(define (hat-main)
  (group
    (hat-shadow-band)
    (hat-body)
    (hat-fur-trim)))