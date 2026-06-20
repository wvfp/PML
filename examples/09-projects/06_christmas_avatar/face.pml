; face.pml - 脸 + 眼睛 + 眉毛 + 腮红 + 嘴
; 脸中心 (500, 560)

(import "palette.pml" as p)

(provide face-main)

; 脸：竖向椭圆（头型）
(define (face-ellipse)
  (ellipse 500 560 220 260
           :fill p/c-skin
           :stroke p/c-stroke
           :stroke-width 8))

; 颈部阴影（在脸下方）
(define (neck-shadow)
  (ellipse 500 830 80 40
           :fill p/c-skin-shadow
           :stroke p/c-stroke
           :stroke-width 5))

; 眉毛 - 两条弯曲的线（用polygon近似弧形粗线）
(define (eyebrows)
  (group
    ; 左眉：在左眼睛上方
    (polygon (list
               (list 330 460)
               (list 355 445)
               (list 390 440)
               (list 415 445)
               (list 415 460)
               (list 390 455)
               (list 355 460))
             :fill p/c-eyelash
             :stroke p/c-stroke
             :stroke-width 0)
    ; 右眉
    (polygon (list
               (list 585 445)
               (list 610 440)
               (list 645 445)
               (list 670 460)
               (list 670 475)
               (list 645 470)
               (list 610 470)
               (list 585 475))
             :fill p/c-eyelash
             :stroke p/c-stroke
             :stroke-width 0)))

; 眼睛 - 大圆眼睛（白眼球 + 彩色虹膜 + 瞳孔 + 高光）
; 左眼中心 (400, 520)，右眼 (600, 520)
(define (eye cx cy scale)
  (let* ((r-outer (* 55 scale))
         (r-iris  (* 35 scale))
         (r-pupil (* 14 scale)))
    (group
      ; 外眼眶（黑色粗线，用一个大椭圆然后在内部填充白色）
      (ellipse cx cy r-outer (* r-outer 1.15)
               :fill p/c-eye-white
               :stroke p/c-stroke
               :stroke-width 10)
      ; 上眼线加粗（阴影条）
      (polygon (list
                 (list (- cx r-outer) cy)
                 (list (- cx (* r-outer 0.7)) (- cy (* r-outer 0.9)))
                 (list cx (- cy (* r-outer 1.05)))
                 (list (+ cx (* r-outer 0.7)) (- cy (* r-outer 0.9)))
                 (list (+ cx r-outer) cy)
                 (list (+ cx (* r-outer 0.8)) (- cy (* r-outer 0.75)))
                 (list cx (- cy (* r-outer 0.9)))
                 (list (- cx (* r-outer 0.8)) (- cy (* r-outer 0.75))))
               :fill p/c-eyelash
               :stroke p/c-stroke
               :stroke-width 0)
      ; 下眼线（较细）
      (polygon (list
                 (list (- cx r-outer) cy)
                 (list (- cx (* r-outer 0.8)) (+ cy (* r-outer 0.8)))
                 (list cx (+ cy (* r-outer 1.0)))
                 (list (+ cx (* r-outer 0.8)) (+ cy (* r-outer 0.8)))
                 (list (+ cx r-outer) cy)
                 (list (+ cx (* r-outer 0.9)) (+ cy (* r-outer 0.7)))
                 (list cx (+ cy (* r-outer 0.85)))
                 (list (- cx (* r-outer 0.9)) (+ cy (* r-outer 0.7))))
               :fill p/c-eyelash
               :stroke p/c-stroke
               :stroke-width 0)
      ; 虹膜（绿色外圈）
      (circle cx cy r-iris
              :fill p/c-iris
              :stroke p/c-stroke
              :stroke-width 3)
      ; 虹膜内深色
      (circle cx cy (* r-iris 0.7)
              :fill p/c-iris-dark
              :stroke p/c-stroke
              :stroke-width 0)
      ; 瞳孔
      (circle cx cy r-pupil
              :fill p/c-stroke
              :stroke p/c-stroke
              :stroke-width 0)
      ; 瞳孔下方小白高光（眼睛下方半圆白）
      (ellipse cx (+ cy (* r-iris 0.5)) (* r-iris 0.55) (* r-iris 0.35)
               :fill p/c-eye-white
               :stroke p/c-stroke
               :stroke-width 0)
      ; 瞳孔上左高光 (小白色矩形近似)
      (ellipse (- cx (* r-iris 0.35)) (- cy (* r-iris 0.45)) (* r-iris 0.25) (* r-iris 0.35)
               :fill p/c-eye-white
               :stroke p/c-stroke
               :stroke-width 0)
      ; 上左更大的高光（椭圆形）
      (ellipse (- cx (* r-iris 0.35)) (- cy (* r-iris 0.45)) (* r-iris 0.18) (* r-iris 0.3)
               :fill p/c-eye-white
               :stroke p/c-stroke
               :stroke-width 0))))

; 腮红 - 两个椭圆
(define (cheeks)
  (group
    (ellipse 340 600 30 20
             :fill p/c-cheek
             :stroke p/c-stroke
             :stroke-width 2)
    (ellipse 660 600 30 20
             :fill p/c-cheek
             :stroke p/c-stroke
             :stroke-width 2)))

; 嘴 - 张开的笑嘴（露出上牙和舌头）
; 嘴中心 (500, 680)
(define (mouth)
  (group
    ; 嘴外圈（椭圆形红唇）
    (ellipse 500 685 70 55
             :fill p/c-mouth
             :stroke p/c-stroke
             :stroke-width 6)
    ; 上排牙齿（白色矩形条）
    (polygon (list
               (list 450 648)
               (list 455 675)
               (list 545 675)
               (list 550 648))
             :fill p/c-eye-white
             :stroke p/c-stroke
             :stroke-width 0)
    ; 舌头（红色椭圆，在下方）
    (ellipse 500 710 35 25
             :fill p/c-tongue
             :stroke p/c-stroke
             :stroke-width 2)))

; 鼻子 - 一个小点
(define (nose)
  (circle 500 630 4
          :fill p/c-stroke
          :stroke p/c-stroke
          :stroke-width 0))

; 整个脸的组合
(define (face-main)
  (group
    (face-ellipse)
    (eyebrows)
    (eye 400 520 1.0)    ; 左眼
    (eye 600 520 1.0)    ; 右眼
    (cheeks)
    (nose)
    (mouth)
    (neck-shadow)))