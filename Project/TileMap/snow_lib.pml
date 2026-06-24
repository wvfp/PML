; snow_lib.pml — 雪地瓦片共享库

(provide snow-shader draw-snow-tile)

; ═══════════════════════════════════════════════════════════════════════════════
; 动漫雪地 shader: 3 色阶白蓝量化
; ═══════════════════════════════════════════════════════════════════════════════

(define snow-noise
  (noise-fractal :seed 34 :octaves 3
                 :freq-x 0.03 :freq-y 0.03
                 :tile-width 300 :tile-height 150))

(define snow-shader
  (quantize-noise snow-noise
    :levels '((0.30 "#A8C0D8")
              (0.65 "#D0E0EE")
              (1.0  "#EEF4FF"))))

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
           :fill "#D0E0EE" :stroke "none" :stroke-width 0))

; ═══════════════════════════════════════════════════════════════════════════════
; 左侧面 (冰冻层)
; ═══════════════════════════════════════════════════════════════════════════════

(define (left-face-snow cx cy w h d)
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
    ; 底层冻土
    (polygon (list (list ax ay) (list bx by) (list c_x c_y) (list dx dy))
             :fill "#4A5565" :stroke "none" :stroke-width 0)

    ; 压实雪层 (锯齿顶边)
    (let ((s1 (pt 0.0 0.45)) (s2 (pt 0.15 0.40)) (s3 (pt 0.30 0.46))
          (s4 (pt 0.48 0.38)) (s5 (pt 0.65 0.44)) (s6 (pt 0.80 0.37))
          (s7 (pt 1.0 0.41)))
      (polygon (list
                 (list (car s1) (list-ref s1 1))
                 (list (car s2) (list-ref s2 1))
                 (list (car s3) (list-ref s3 1))
                 (list (car s4) (list-ref s4 1))
                 (list (car s5) (list-ref s5 1))
                 (list (car s6) (list-ref s6 1))
                 (list (car s7) (list-ref s7 1))
                 (list bx by) (list c_x c_y) (list dx dy))
               :fill "#6A7A8A" :stroke "none"))

    ; 表层冰/雪 (锯齿顶边)
    (let ((t1 (pt 0.0 0.45)) (t2 (pt 0.15 0.40)) (t3 (pt 0.30 0.46))
          (t4 (pt 0.48 0.38)) (t5 (pt 0.65 0.44)) (t6 (pt 0.80 0.37))
          (t7 (pt 1.0 0.41))
          (b1 (pt 0.0 0.05)) (b2 (pt 0.18 0.02)) (b3 (pt 0.35 0.07))
          (b4 (pt 0.55 0.03)) (b5 (pt 0.75 0.06)) (b6 (pt 1.0 0.03)))
      (polygon (list
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
                 (list (car t7) (list-ref t7 1)))
               :fill "#3A4A5A" :stroke "none"))

    ; 冰裂纹
    (let ((p1 (pt 0.10 0.52)) (p2 (pt 0.35 0.49)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#2A3A4A" :stroke-width 1.2))
    (let ((p1 (pt 0.42 0.65)) (p2 (pt 0.70 0.61)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#2A3A4A" :stroke-width 1.0))
    (let ((p1 (pt 0.62 0.80)) (p2 (pt 0.88 0.76)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#2A3A4A" :stroke-width 0.8))

    ; 冰晶亮点
    (let ((p (pt 0.22 0.55)))
      (circle (car p) (list-ref p 1) 2 :fill "#8AA0B8" :stroke "none"))
    (let ((p (pt 0.55 0.70)))
      (circle (car p) (list-ref p 1) 1.8 :fill "#8AA0B8" :stroke "none"))
    (let ((p (pt 0.80 0.85)))
      (circle (car p) (list-ref p 1) 1.5 :fill "#7A90A8" :stroke "none"))))

; ═══════════════════════════════════════════════════════════════════════════════
; 右侧面 (偏暗)
; ═══════════════════════════════════════════════════════════════════════════════

(define (right-face-snow cx cy w h d)
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
             :fill "#3A4555" :stroke "none" :stroke-width 0)

    (let ((s1 (pt 0.0 0.43)) (s2 (pt 0.15 0.38)) (s3 (pt 0.30 0.44))
          (s4 (pt 0.48 0.36)) (s5 (pt 0.65 0.42)) (s6 (pt 0.80 0.35))
          (s7 (pt 1.0 0.39)))
      (polygon (list
                 (list (car s1) (list-ref s1 1))
                 (list (car s2) (list-ref s2 1))
                 (list (car s3) (list-ref s3 1))
                 (list (car s4) (list-ref s4 1))
                 (list (car s5) (list-ref s5 1))
                 (list (car s6) (list-ref s6 1))
                 (list (car s7) (list-ref s7 1))
                 (list bx by) (list c_x c_y) (list dx dy))
               :fill "#5A6A7A" :stroke "none"))

    (let ((t1 (pt 0.0 0.43)) (t2 (pt 0.15 0.38)) (t3 (pt 0.30 0.44))
          (t4 (pt 0.48 0.36)) (t5 (pt 0.65 0.42)) (t6 (pt 0.80 0.35))
          (t7 (pt 1.0 0.39))
          (b1 (pt 0.0 0.03)) (b2 (pt 0.18 0.00)) (b3 (pt 0.35 0.05))
          (b4 (pt 0.55 0.01)) (b5 (pt 0.75 0.04)) (b6 (pt 1.0 0.01)))
      (polygon (list
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
                 (list (car t7) (list-ref t7 1)))
               :fill "#2A3A4A" :stroke "none"))

    (let ((p1 (pt 0.10 0.50)) (p2 (pt 0.35 0.47)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#1A2A3A" :stroke-width 1.2))
    (let ((p1 (pt 0.42 0.63)) (p2 (pt 0.70 0.59)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#1A2A3A" :stroke-width 1.0))
    (let ((p1 (pt 0.62 0.78)) (p2 (pt 0.88 0.74)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#1A2A3A" :stroke-width 0.8))

    (let ((p (pt 0.22 0.53)))
      (circle (car p) (list-ref p 1) 1.8 :fill "#7A90A8" :stroke "none"))
    (let ((p (pt 0.55 0.68)))
      (circle (car p) (list-ref p 1) 1.5 :fill "#7A90A8" :stroke "none"))
    (let ((p (pt 0.80 0.83)))
      (circle (car p) (list-ref p 1) 1.2 :fill "#6A8098" :stroke "none"))))

; ═══════════════════════════════════════════════════════════════════════════════
; 完整雪地瓦片
; ═══════════════════════════════════════════════════════════════════════════════

(define (draw-snow-tile cx cy tw th td shader)
  (group
    (left-face-snow cx cy tw th td)
    (right-face-snow cx cy tw th td)
    (apply-shader! (diamond cx cy tw th) shader)))
