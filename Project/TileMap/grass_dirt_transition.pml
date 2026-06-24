; grass_dirt_transition.pml — 草地→泥土过渡瓦片
; 顶面草地shader，侧面使用泥土颜色

(provide draw-grass-dirt-transition)

(import "grass_lib.pml" as grass)

; 本地 fp
(define (fp ax ay bx by dx dy cx_ cy_ u v)
  (define t_x (+ ax (* u (- bx ax))))
  (define t_y (+ ay (* u (- by ay))))
  (define b_x (+ dx (* u (- cx_ dx))))
  (define b_y (+ dy (* u (- cy_ dy))))
  (list (+ t_x (* v (- b_x t_x)))
        (+ t_y (* v (- b_y t_y)))))

; 本地 diamond
(define (diamond cx cy w h)
  (polygon (list
             (list cx (- cy (/ h 2)))
             (list (+ cx (/ w 2)) cy)
             (list cx (+ cy (/ h 2)))
             (list (- cx (/ w 2)) cy))
           :fill "#3D8C2E" :stroke "none" :stroke-width 0))

; ═══════════════════════════════════════════════════════════════════════════════
; 左侧面: 泥土色 (与 dirt_lib 一致: #8B6B45 心土)
; ═══════════════════════════════════════════════════════════════════════════════

(define (left-face-dirt-color cx cy w h d)
  (define ax (- cx (/ w 2)))
  (define ay cy)
  (define bx cx)
  (define by (+ cy (/ h 2)))
  (define c_x bx)
  (define c_y (+ by d))
  (define dx ax)
  (define dy (+ ay d))
  (define (pt u v) (fp ax ay bx by dx dy c_x c_y u v))

  (group
    (polygon (list (list ax ay) (list bx by) (list c_x c_y) (list dx dy))
             :fill "#8B6B45" :stroke "none" :stroke-width 0)

    (let ((t1 (pt 0.0 0.43)) (t2 (pt 0.15 0.38)) (t3 (pt 0.30 0.44))
          (t4 (pt 0.45 0.37)) (t5 (pt 0.60 0.42)) (t6 (pt 0.75 0.36))
          (t7 (pt 0.90 0.40)) (t8 (pt 1.0 0.38))
          (b1 (pt 0.0 0.12)) (b2 (pt 0.15 0.10)) (b3 (pt 0.30 0.14))
          (b4 (pt 0.45 0.09)) (b5 (pt 0.60 0.13)) (b6 (pt 0.75 0.08))
          (b7 (pt 0.90 0.11)) (b8 (pt 1.0 0.09)))
      (polygon (list
                 (list (car b8) (list-ref b8 1))
                 (list (car b7) (list-ref b7 1))
                 (list (car b6) (list-ref b6 1))
                 (list (car b5) (list-ref b5 1))
                 (list (car b4) (list-ref b4 1))
                 (list (car b3) (list-ref b3 1))
                 (list (car b2) (list-ref b2 1))
                 (list (car b1) (list-ref b1 1))
                 (list (car t1) (list-ref t1 1))
                 (list (car t2) (list-ref t2 1))
                 (list (car t3) (list-ref t3 1))
                 (list (car t4) (list-ref t4 1))
                 (list (car t5) (list-ref t5 1))
                 (list (car t6) (list-ref t6 1))
                 (list (car t7) (list-ref t7 1))
                 (list (car t8) (list-ref t8 1)))
               :fill "#4A3520" :stroke "none"))

    (let ((p1 (pt 0.12 0.52)) (p2 (pt 0.38 0.48)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#3A2815" :stroke-width 1.5))
    (let ((p1 (pt 0.45 0.65)) (p2 (pt 0.72 0.60)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#3A2815" :stroke-width 1.2))
    (let ((p1 (pt 0.65 0.80)) (p2 (pt 0.90 0.76)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#3A2815" :stroke-width 1.0))

    (let ((p (pt 0.25 0.55)))
      (ellipse (car p) (list-ref p 1) 7 4 :fill "#8A7558" :stroke "none"))
    (let ((p (pt 0.60 0.70)))
      (ellipse (car p) (list-ref p 1) 6 3 :fill "#7A6548" :stroke "none"))

    (let ((p (pt 0.18 0.58)))
      (circle (car p) (list-ref p 1) 2.5 :fill "#6A5538" :stroke "none"))
    (let ((p (pt 0.48 0.68)))
      (circle (car p) (list-ref p 1) 2 :fill "#5A4528" :stroke "none"))
    (let ((p (pt 0.78 0.85)))
      (circle (car p) (list-ref p 1) 2 :fill "#5A4528" :stroke "none"))))

; ═══════════════════════════════════════════════════════════════════════════════
; 右侧面: 泥土色偏暗 (与 dirt_lib 一致: #7A5B35)
; ═══════════════════════════════════════════════════════════════════════════════

(define (right-face-dirt-color cx cy w h d)
  (define ax (+ cx (/ w 2)))
  (define ay cy)
  (define bx cx)
  (define by (+ cy (/ h 2)))
  (define c_x bx)
  (define c_y (+ by d))
  (define dx ax)
  (define dy (+ ay d))
  (define (pt u v) (fp ax ay bx by dx dy c_x c_y u v))

  (group
    (polygon (list (list ax ay) (list bx by) (list c_x c_y) (list dx dy))
             :fill "#7A5B35" :stroke "none" :stroke-width 0)

    (let ((t1 (pt 0.0 0.43)) (t2 (pt 0.15 0.38)) (t3 (pt 0.30 0.44))
          (t4 (pt 0.45 0.37)) (t5 (pt 0.60 0.42)) (t6 (pt 0.75 0.36))
          (t7 (pt 0.90 0.40)) (t8 (pt 1.0 0.38))
          (b1 (pt 0.0 0.06)) (b2 (pt 0.15 0.03)) (b3 (pt 0.30 0.08))
          (b4 (pt 0.50 0.04)) (b5 (pt 0.70 0.07)) (b6 (pt 0.85 0.03))
          (b7 (pt 1.0 0.05)))
      (polygon (list
                 (list (car b7) (list-ref b7 1))
                 (list (car b6) (list-ref b6 1))
                 (list (car b5) (list-ref b5 1))
                 (list (car b4) (list-ref b4 1))
                 (list (car b3) (list-ref b3 1))
                 (list (car b2) (list-ref b2 1))
                 (list (car b1) (list-ref b1 1))
                 (list (car t1) (list-ref t1 1))
                 (list (car t2) (list-ref t2 1))
                 (list (car t3) (list-ref t3 1))
                 (list (car t4) (list-ref t4 1))
                 (list (car t5) (list-ref t5 1))
                 (list (car t6) (list-ref t6 1))
                 (list (car t7) (list-ref t7 1))
                 (list (car t8) (list-ref t8 1)))
               :fill "#3A2815" :stroke "none"))

    (let ((p1 (pt 0.12 0.50)) (p2 (pt 0.38 0.46)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#2A1A0A" :stroke-width 1.5))
    (let ((p1 (pt 0.45 0.63)) (p2 (pt 0.72 0.58)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#2A1A0A" :stroke-width 1.2))
    (let ((p1 (pt 0.65 0.78)) (p2 (pt 0.90 0.74)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#2A1A0A" :stroke-width 1.0))

    (let ((p (pt 0.25 0.53)))
      (ellipse (car p) (list-ref p 1) 6 3 :fill "#7A6548" :stroke "none"))
    (let ((p (pt 0.60 0.68)))
      (ellipse (car p) (list-ref p 1) 5 3 :fill "#6A5538" :stroke "none"))

    (let ((p (pt 0.18 0.56)))
      (circle (car p) (list-ref p 1) 2 :fill "#5A4528" :stroke "none"))
    (let ((p (pt 0.48 0.66)))
      (circle (car p) (list-ref p 1) 1.8 :fill "#4A3518" :stroke "none"))
    (let ((p (pt 0.78 0.83)))
      (circle (car p) (list-ref p 1) 1.8 :fill "#4A3518" :stroke "none"))))

; ═══════════════════════════════════════════════════════════════════════════════
; 过渡瓦片: 顶面草地 + 两侧泥土色
; ═══════════════════════════════════════════════════════════════════════════════

(define (draw-grass-dirt-transition cx cy tw th td shader)
  (group
    (left-face-dirt-color cx cy tw th td)
    (right-face-dirt-color cx cy tw th td)
    (apply-shader! (diamond cx cy tw th) shader)))
