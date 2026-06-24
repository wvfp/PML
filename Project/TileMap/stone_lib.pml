; stone_lib.pml — 石头/岩石瓦片共享库

(provide stone-shader draw-stone-tile)

; ═══════════════════════════════════════════════════════════════════════════════
; 动漫岩石 shader: 3 色阶灰色调量化
; ═══════════════════════════════════════════════════════════════════════════════

(define stone-noise
  (noise-fractal :seed 91 :octaves 3
                 :freq-x 0.03 :freq-y 0.03
                 :tile-width 300 :tile-height 150))

(define stone-shader
  (quantize-noise stone-noise
    :levels '((0.30 "#3A3B42")
              (0.65 "#5A5C65")
              (1.0  "#808590"))))

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
           :fill "#5A5C65" :stroke "none" :stroke-width 0))

; ═══════════════════════════════════════════════════════════════════════════════
; 左侧面 (层状岩石)
; ═══════════════════════════════════════════════════════════════════════════════

(define (left-face-stone cx cy w h d)
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
    ; 底层深色岩石
    (polygon (list (list ax ay) (list bx by) (list c_x c_y) (list dx dy))
             :fill "#3A3530" :stroke "none" :stroke-width 0)

    ; 中层岩石 (锯齿顶边)
    (let ((s1 (pt 0.0 0.50)) (s2 (pt 0.12 0.46)) (s3 (pt 0.28 0.52))
          (s4 (pt 0.42 0.44)) (s5 (pt 0.58 0.50)) (s6 (pt 0.72 0.43))
          (s7 (pt 0.88 0.48)) (s8 (pt 1.0 0.45)))
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
               :fill "#4A4540" :stroke "none"))

    ; 表层岩石 (锯齿顶边)
    (let ((t1 (pt 0.0 0.50)) (t2 (pt 0.12 0.46)) (t3 (pt 0.28 0.52))
          (t4 (pt 0.42 0.44)) (t5 (pt 0.58 0.50)) (t6 (pt 0.72 0.43))
          (t7 (pt 0.88 0.48)) (t8 (pt 1.0 0.45))
          (b1 (pt 0.0 0.06)) (b2 (pt 0.20 0.03)) (b3 (pt 0.40 0.08))
          (b4 (pt 0.60 0.04)) (b5 (pt 0.80 0.07)) (b6 (pt 1.0 0.04)))
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
                 (list (car t7) (list-ref t7 1))
                 (list (car t8) (list-ref t8 1)))
               :fill "#2E2A28" :stroke "none"))

    ; 岩石裂纹
    (let ((p1 (pt 0.08 0.55)) (p2 (pt 0.32 0.52)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#1A1815" :stroke-width 1.5))
    (let ((p1 (pt 0.40 0.68)) (p2 (pt 0.68 0.64)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#1A1815" :stroke-width 1.2))
    (let ((p1 (pt 0.60 0.82)) (p2 (pt 0.88 0.78)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#1A1815" :stroke-width 1.0))

    ; 层理线 (水平)
    (let ((p1 (pt 0.05 0.72)) (p2 (pt 0.45 0.70)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#5A5550" :stroke-width 0.8))
    (let ((p1 (pt 0.50 0.88)) (p2 (pt 0.92 0.86)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#5A5550" :stroke-width 0.8))

    ; 矿物斑点
    (let ((p (pt 0.22 0.58)))
      (circle (car p) (list-ref p 1) 3 :fill "#6A6560" :stroke "none"))
    (let ((p (pt 0.55 0.74)))
      (circle (car p) (list-ref p 1) 2.5 :fill "#5A5550" :stroke "none"))
    (let ((p (pt 0.80 0.88)))
      (circle (car p) (list-ref p 1) 2 :fill "#5A5550" :stroke "none"))))

; ═══════════════════════════════════════════════════════════════════════════════
; 右侧面 (偏暗)
; ═══════════════════════════════════════════════════════════════════════════════

(define (right-face-stone cx cy w h d)
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
             :fill "#2A2520" :stroke "none" :stroke-width 0)

    (let ((s1 (pt 0.0 0.48)) (s2 (pt 0.12 0.44)) (s3 (pt 0.28 0.50))
          (s4 (pt 0.42 0.42)) (s5 (pt 0.58 0.48)) (s6 (pt 0.72 0.41))
          (s7 (pt 0.88 0.46)) (s8 (pt 1.0 0.43)))
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
               :fill "#3A3530" :stroke "none"))

    (let ((t1 (pt 0.0 0.48)) (t2 (pt 0.12 0.44)) (t3 (pt 0.28 0.50))
          (t4 (pt 0.42 0.42)) (t5 (pt 0.58 0.48)) (t6 (pt 0.72 0.41))
          (t7 (pt 0.88 0.46)) (t8 (pt 1.0 0.43))
          (b1 (pt 0.0 0.04)) (b2 (pt 0.20 0.01)) (b3 (pt 0.40 0.06))
          (b4 (pt 0.60 0.02)) (b5 (pt 0.80 0.05)) (b6 (pt 1.0 0.02)))
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
                 (list (car t7) (list-ref t7 1))
                 (list (car t8) (list-ref t8 1)))
               :fill "#1E1A18" :stroke "none"))

    (let ((p1 (pt 0.08 0.53)) (p2 (pt 0.32 0.50)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#0E0C0A" :stroke-width 1.5))
    (let ((p1 (pt 0.40 0.66)) (p2 (pt 0.68 0.62)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#0E0C0A" :stroke-width 1.2))
    (let ((p1 (pt 0.60 0.80)) (p2 (pt 0.88 0.76)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#0E0C0A" :stroke-width 1.0))

    (let ((p1 (pt 0.05 0.70)) (p2 (pt 0.45 0.68)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#4A4540" :stroke-width 0.8))
    (let ((p1 (pt 0.50 0.86)) (p2 (pt 0.92 0.84)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#4A4540" :stroke-width 0.8))

    (let ((p (pt 0.22 0.56)))
      (circle (car p) (list-ref p 1) 2.5 :fill "#5A5550" :stroke "none"))
    (let ((p (pt 0.55 0.72)))
      (circle (car p) (list-ref p 1) 2 :fill "#4A4540" :stroke "none"))
    (let ((p (pt 0.80 0.86)))
      (circle (car p) (list-ref p 1) 1.8 :fill "#4A4540" :stroke "none"))))

; ═══════════════════════════════════════════════════════════════════════════════
; 完整岩石瓦片
; ═══════════════════════════════════════════════════════════════════════════════

(define (draw-stone-tile cx cy tw th td shader)
  (group
    (left-face-stone cx cy tw th td)
    (right-face-stone cx cy tw th td)
    (apply-shader! (diamond cx cy tw th) shader)))
