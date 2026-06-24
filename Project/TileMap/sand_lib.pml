; sand_lib.pml — 沙地瓦片共享库

(provide sand-shader draw-sand-tile)

; ═══════════════════════════════════════════════════════════════════════════════
; 动漫沙地 shader: 3 色阶黄褐量化
; ═══════════════════════════════════════════════════════════════════════════════

(define sand-noise
  (noise-fractal :seed 58 :octaves 3
                 :freq-x 0.03 :freq-y 0.03
                 :tile-width 300 :tile-height 150))

(define sand-shader
  (quantize-noise sand-noise
    :levels '((0.30 "#A08050")
              (0.65 "#C8A870")
              (1.0  "#E8D098"))))

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
           :fill "#C8A870" :stroke "none" :stroke-width 0))

; ═══════════════════════════════════════════════════════════════════════════════
; 左侧面 (沙层)
; ═══════════════════════════════════════════════════════════════════════════════

(define (left-face-sand cx cy w h d)
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
    ; 底层
    (polygon (list (list ax ay) (list bx by) (list c_x c_y) (list dx dy))
             :fill "#7A6545" :stroke "none" :stroke-width 0)

    ; 中层沙 (锯齿顶边)
    (let ((s1 (pt 0.0 0.48)) (s2 (pt 0.18 0.43)) (s3 (pt 0.35 0.49))
          (s4 (pt 0.52 0.42)) (s5 (pt 0.70 0.47)) (s6 (pt 0.85 0.41))
          (s7 (pt 1.0 0.44)))
      (polygon (list
                 (list (car s1) (list-ref s1 1))
                 (list (car s2) (list-ref s2 1))
                 (list (car s3) (list-ref s3 1))
                 (list (car s4) (list-ref s4 1))
                 (list (car s5) (list-ref s5 1))
                 (list (car s6) (list-ref s6 1))
                 (list (car s7) (list-ref s7 1))
                 (list bx by) (list c_x c_y) (list dx dy))
               :fill "#8A7555" :stroke "none"))

    ; 表层沙
    (let ((t1 (pt 0.0 0.48)) (t2 (pt 0.18 0.43)) (t3 (pt 0.35 0.49))
          (t4 (pt 0.52 0.42)) (t5 (pt 0.70 0.47)) (t6 (pt 0.85 0.41))
          (t7 (pt 1.0 0.44))
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
                 (list (car t7) (list-ref t7 1)))
               :fill "#5A4A35" :stroke "none"))

    ; 沙层理线
    (let ((p1 (pt 0.08 0.58)) (p2 (pt 0.48 0.55)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#A09068" :stroke-width 0.8))
    (let ((p1 (pt 0.30 0.72)) (p2 (pt 0.72 0.68)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#A09068" :stroke-width 0.8))
    (let ((p1 (pt 0.55 0.85)) (p2 (pt 0.92 0.82)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#A09068" :stroke-width 0.8))

    ; 沙砾
    (let ((p (pt 0.20 0.55)))
      (circle (car p) (list-ref p 1) 2 :fill "#B0A078" :stroke "none"))
    (let ((p (pt 0.50 0.68)))
      (circle (car p) (list-ref p 1) 1.8 :fill "#B0A078" :stroke "none"))
    (let ((p (pt 0.75 0.82)))
      (circle (car p) (list-ref p 1) 1.5 :fill "#B0A078" :stroke "none"))))

; ═══════════════════════════════════════════════════════════════════════════════
; 右侧面 (偏暗)
; ═══════════════════════════════════════════════════════════════════════════════

(define (right-face-sand cx cy w h d)
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
             :fill "#6A5535" :stroke "none" :stroke-width 0)

    (let ((s1 (pt 0.0 0.46)) (s2 (pt 0.18 0.41)) (s3 (pt 0.35 0.47))
          (s4 (pt 0.52 0.40)) (s5 (pt 0.70 0.45)) (s6 (pt 0.85 0.39))
          (s7 (pt 1.0 0.42)))
      (polygon (list
                 (list (car s1) (list-ref s1 1))
                 (list (car s2) (list-ref s2 1))
                 (list (car s3) (list-ref s3 1))
                 (list (car s4) (list-ref s4 1))
                 (list (car s5) (list-ref s5 1))
                 (list (car s6) (list-ref s6 1))
                 (list (car s7) (list-ref s7 1))
                 (list bx by) (list c_x c_y) (list dx dy))
               :fill "#7A6545" :stroke "none"))

    (let ((t1 (pt 0.0 0.46)) (t2 (pt 0.18 0.41)) (t3 (pt 0.35 0.47))
          (t4 (pt 0.52 0.40)) (t5 (pt 0.70 0.45)) (t6 (pt 0.85 0.39))
          (t7 (pt 1.0 0.42))
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
                 (list (car t7) (list-ref t7 1)))
               :fill "#4A3A28" :stroke "none"))

    (let ((p1 (pt 0.08 0.56)) (p2 (pt 0.48 0.53)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#8A7A58" :stroke-width 0.8))
    (let ((p1 (pt 0.30 0.70)) (p2 (pt 0.72 0.66)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#8A7A58" :stroke-width 0.8))
    (let ((p1 (pt 0.55 0.83)) (p2 (pt 0.92 0.80)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#8A7A58" :stroke-width 0.8))

    (let ((p (pt 0.20 0.53)))
      (circle (car p) (list-ref p 1) 1.8 :fill "#9A8A68" :stroke "none"))
    (let ((p (pt 0.50 0.66)))
      (circle (car p) (list-ref p 1) 1.5 :fill "#9A8A68" :stroke "none"))
    (let ((p (pt 0.75 0.80)))
      (circle (car p) (list-ref p 1) 1.2 :fill "#9A8A68" :stroke "none"))))

; ═══════════════════════════════════════════════════════════════════════════════
; 完整沙地瓦片
; ═══════════════════════════════════════════════════════════════════════════════

(define (draw-sand-tile cx cy tw th td shader)
  (group
    (left-face-sand cx cy tw th td)
    (right-face-sand cx cy tw th td)
    (apply-shader! (diamond cx cy tw th) shader)))
