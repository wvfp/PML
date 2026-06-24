; dirt_lib.pml — 泥土/土路瓦片共享库
; 动漫风顶面 + 写实侧面

(provide dirt-shader draw-dirt-tile)

; ═══════════════════════════════════════════════════════════════════════════════
; 动漫泥土 shader: 3 色阶棕色调量化
; ═══════════════════════════════════════════════════════════════════════════════

(define dirt-noise
  (noise-fractal :seed 73 :octaves 3
                 :freq-x 0.03 :freq-y 0.03
                 :tile-width 300 :tile-height 150))

(define dirt-shader
  (quantize-noise dirt-noise
    :levels '((0.30 "#4A3520")
              (0.65 "#7A5C38")
              (1.0  "#A88558"))))

; ═══════════════════════════════════════════════════════════════════════════════
; 面内插值
; ═══════════════════════════════════════════════════════════════════════════════

(define (fp ax ay bx by dx dy cx_ cy_ u v)
  (define t_x (+ ax (* u (- bx ax))))
  (define t_y (+ ay (* u (- by ay))))
  (define b_x (+ dx (* u (- cx_ dx))))
  (define b_y (+ dy (* u (- cy_ dy))))
  (list (+ t_x (* v (- b_x t_x)))
        (+ t_y (* v (- b_y t_y)))))

(define (diamond cx cy w h)
  (polygon (list
             (list cx (- cy (/ h 2)))
             (list (+ cx (/ w 2)) cy)
             (list cx (+ cy (/ h 2)))
             (list (- cx (/ w 2)) cy))
           :fill "#7A5C38" :stroke "none" :stroke-width 0))

; ═══════════════════════════════════════════════════════════════════════════════
; 左侧面 (分层土壤)
; ═══════════════════════════════════════════════════════════════════════════════

(define (left-face-dirt cx cy w h d)
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
    ; 底层岩石
    (polygon (list (list ax ay) (list bx by) (list c_x c_y) (list dx dy))
             :fill "#5A4A3A" :stroke "none" :stroke-width 0)

    ; 心土层 (锯齿顶边)
    (let ((s1 (pt 0.0 0.45)) (s2 (pt 0.15 0.40)) (s3 (pt 0.30 0.46))
          (s4 (pt 0.45 0.39)) (s5 (pt 0.60 0.44)) (s6 (pt 0.75 0.38))
          (s7 (pt 0.90 0.42)) (s8 (pt 1.0 0.40)))
      (polygon (list
                 (list (car s1) (list-ref s1 1))
                 (list (car s2) (list-ref s2 1))
                 (list (car s3) (list-ref s3 1))
                 (list (car s4) (list-ref s4 1))
                 (list (car s5) (list-ref s5 1))
                 (list (car s6) (list-ref s6 1))
                 (list (car s7) (list-ref s7 1))
                 (list (car s8) (list-ref s8 1))
                 (list bx by) (list c_x c_y) (list dx dy))
               :fill "#6B4E2E" :stroke "none"))

    ; 表层泥土 (锯齿顶边)
    (let ((t1 (pt 0.0 0.45)) (t2 (pt 0.15 0.40)) (t3 (pt 0.30 0.46))
          (t4 (pt 0.45 0.39)) (t5 (pt 0.60 0.44)) (t6 (pt 0.75 0.38))
          (t7 (pt 0.90 0.42)) (t8 (pt 1.0 0.40))
          (b1 (pt 0.0 0.08)) (b2 (pt 0.15 0.05)) (b3 (pt 0.30 0.10))
          (b4 (pt 0.50 0.06)) (b5 (pt 0.70 0.09)) (b6 (pt 0.85 0.05))
          (b7 (pt 1.0 0.07)))
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
               :fill "#4A3520" :stroke "none"))

    ; 裂纹
    (let ((p1 (pt 0.12 0.52)) (p2 (pt 0.38 0.48)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#3A2815" :stroke-width 1.5))
    (let ((p1 (pt 0.45 0.65)) (p2 (pt 0.72 0.60)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#3A2815" :stroke-width 1.2))
    (let ((p1 (pt 0.65 0.80)) (p2 (pt 0.90 0.76)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#3A2815" :stroke-width 1.0))

    ; 卵石
    (let ((p (pt 0.25 0.55)))
      (ellipse (car p) (list-ref p 1) 7 4 :fill "#8A7558" :stroke "none"))
    (let ((p (pt 0.60 0.70)))
      (ellipse (car p) (list-ref p 1) 6 3 :fill "#7A6548" :stroke "none"))

    ; 碎石
    (let ((p (pt 0.18 0.58)))
      (circle (car p) (list-ref p 1) 2.5 :fill "#6A5538" :stroke "none"))
    (let ((p (pt 0.48 0.68)))
      (circle (car p) (list-ref p 1) 2 :fill "#5A4528" :stroke "none"))
    (let ((p (pt 0.78 0.85)))
      (circle (car p) (list-ref p 1) 2 :fill "#5A4528" :stroke "none"))

    ; 岩石层理
    (let ((p1 (pt 0.10 0.80)) (p2 (pt 0.40 0.78)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#4A3A2A" :stroke-width 1.0))
    (let ((p (pt 0.55 0.88)))
      (circle (car p) (list-ref p 1) 2 :fill "#6A5A48" :stroke "none"))))

; ═══════════════════════════════════════════════════════════════════════════════
; 右侧面 (偏暗)
; ═══════════════════════════════════════════════════════════════════════════════

(define (right-face-dirt cx cy w h d)
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
             :fill "#4A3A2A" :stroke "none" :stroke-width 0)

    (let ((s1 (pt 0.0 0.43)) (s2 (pt 0.15 0.38)) (s3 (pt 0.30 0.44))
          (s4 (pt 0.45 0.37)) (s5 (pt 0.60 0.42)) (s6 (pt 0.75 0.36))
          (s7 (pt 0.90 0.40)) (s8 (pt 1.0 0.38)))
      (polygon (list
                 (list (car s1) (list-ref s1 1))
                 (list (car s2) (list-ref s2 1))
                 (list (car s3) (list-ref s3 1))
                 (list (car s4) (list-ref s4 1))
                 (list (car s5) (list-ref s5 1))
                 (list (car s6) (list-ref s6 1))
                 (list (car s7) (list-ref s7 1))
                 (list (car s8) (list-ref s8 1))
                 (list bx by) (list c_x c_y) (list dx dy))
               :fill "#5A3E22" :stroke "none"))

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
      (circle (car p) (list-ref p 1) 1.8 :fill "#4A3518" :stroke "none"))

    (let ((p1 (pt 0.10 0.78)) (p2 (pt 0.40 0.76)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#3A2A1A" :stroke-width 1.0))
    (let ((p (pt 0.55 0.86)))
      (circle (car p) (list-ref p 1) 1.8 :fill "#5A4A38" :stroke "none"))))

; ═══════════════════════════════════════════════════════════════════════════════
; 完整泥土瓦片
; ═══════════════════════════════════════════════════════════════════════════════

(define (draw-dirt-tile cx cy tw th td shader)
  (group
    (left-face-dirt cx cy tw th td)
    (right-face-dirt cx cy tw th td)
    (apply-shader! (diamond cx cy tw th) shader)))
