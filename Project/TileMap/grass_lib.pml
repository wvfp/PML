; grass_lib.pml — 草地瓦片共享库
; 提供：动漫草地 shader、多层土壤侧面、draw-grass-tile

(provide grass-shader draw-grass-tile)

; ═══════════════════════════════════════════════════════════════════════════════
; 动漫草地 shader: 无缝噪声 + 3 色阶量化
; ═══════════════════════════════════════════════════════════════════════════════

(define grass-noise
  (noise-fractal :seed 42 :octaves 3
                 :freq-x 0.03 :freq-y 0.03
                 :tile-width 300 :tile-height 150))

(define grass-shader
  (quantize-noise grass-noise
    :levels '((0.30 "#267A16")
              (0.65 "#449e2d")
              (1.0  "#74c44f"))))

; ═══════════════════════════════════════════════════════════════════════════════
; 面内插值 (u=0~1 左→右, v=0~1 顶→底)
; ═══════════════════════════════════════════════════════════════════════════════

(define (fp ax ay bx by dx dy cx_ cy_ u v)
  (define t_x (+ ax (* u (- bx ax))))
  (define t_y (+ ay (* u (- by ay))))
  (define b_x (+ dx (* u (- cx_ dx))))
  (define b_y (+ dy (* u (- cy_ dy))))
  (list (+ t_x (* v (- b_x t_x)))
        (+ t_y (* v (- b_y t_y)))))

; ═══════════════════════════════════════════════════════════════════════════════
; 顶面菱形
; ═══════════════════════════════════════════════════════════════════════════════

(define (diamond cx cy w h)
  (polygon (list
             (list cx (- cy (/ h 2)))
             (list (+ cx (/ w 2)) cy)
             (list cx (+ cy (/ h 2)))
             (list (- cx (/ w 2)) cy))
           :fill "#3D8C2E" :stroke "none" :stroke-width 0))

; ═══════════════════════════════════════════════════════════════════════════════
; 左侧面 (多层土壤 + 写实锯齿过渡)
; ═══════════════════════════════════════════════════════════════════════════════

(define (left-face-soil cx cy w h d)
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
    ; ── 岩石底层 ──
    (polygon (list (list ax ay) (list bx by) (list c_x c_y) (list dx dy))
             :fill "#6B5B4A" :stroke "none" :stroke-width 0)

    ; ── 心土层 ──
    (let ((s1 (pt 0.0 0.42)) (s2 (pt 0.12 0.38)) (s3 (pt 0.25 0.43))
          (s4 (pt 0.38 0.37)) (s5 (pt 0.52 0.41)) (s6 (pt 0.65 0.36))
          (s7 (pt 0.78 0.40)) (s8 (pt 0.90 0.35)) (s9 (pt 1.0 0.38)))
      (polygon (list
                 (list (car s1) (list-ref s1 1))
                 (list (car s2) (list-ref s2 1))
                 (list (car s3) (list-ref s3 1))
                 (list (car s4) (list-ref s4 1))
                 (list (car s5) (list-ref s5 1))
                 (list (car s6) (list-ref s6 1))
                 (list (car s7) (list-ref s7 1))
                 (list (car s8) (list-ref s8 1))
                 (list (car s9) (list-ref s9 1))
                 (list bx by) (list c_x c_y) (list dx dy))
               :fill "#8B6B45" :stroke "none"))

    ; ── 腐殖土层 ──
    (let ((t1 (pt 0.0 0.42)) (t2 (pt 0.12 0.38)) (t3 (pt 0.25 0.43))
          (t4 (pt 0.38 0.37)) (t5 (pt 0.52 0.41)) (t6 (pt 0.65 0.36))
          (t7 (pt 0.78 0.40)) (t8 (pt 0.90 0.35)) (t9 (pt 1.0 0.38))
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
                 (list (car t8) (list-ref t8 1))
                 (list (car t9) (list-ref t9 1)))
               :fill "#4A3520" :stroke "none"))

    ; ── 草层覆盖 ──
    (let ((g1  (pt 0.00 0.11)) (g2  (pt 0.07 0.07)) (g3  (pt 0.14 0.13))
          (g4  (pt 0.21 0.06)) (g5  (pt 0.28 0.10)) (g6  (pt 0.35 0.05))
          (g7  (pt 0.42 0.12)) (g8  (pt 0.49 0.07)) (g9  (pt 0.56 0.11))
          (g10 (pt 0.63 0.04)) (g11 (pt 0.70 0.09)) (g12 (pt 0.77 0.06))
          (g13 (pt 0.84 0.10)) (g14 (pt 0.91 0.05)) (g15 (pt 1.00 0.08)))
      (polygon (list
                 (list ax ay) (list bx by)
                 (list (car g15) (list-ref g15 1))
                 (list (car g14) (list-ref g14 1))
                 (list (car g13) (list-ref g13 1))
                 (list (car g12) (list-ref g12 1))
                 (list (car g11) (list-ref g11 1))
                 (list (car g10) (list-ref g10 1))
                 (list (car g9)  (list-ref g9 1))
                 (list (car g8)  (list-ref g8 1))
                 (list (car g7)  (list-ref g7 1))
                 (list (car g6)  (list-ref g6 1))
                 (list (car g5)  (list-ref g5 1))
                 (list (car g4)  (list-ref g4 1))
                 (list (car g3)  (list-ref g3 1))
                 (list (car g2)  (list-ref g2 1))
                 (list (car g1)  (list-ref g1 1)))
               :fill "#2D6B1E" :stroke "none"))

    ; ── 草层亮色叠加 ──
    (let ((g1 (pt 0.00 0.06)) (g2 (pt 0.10 0.03)) (g3 (pt 0.20 0.07))
          (g4 (pt 0.30 0.02)) (g5 (pt 0.40 0.05)) (g6 (pt 0.50 0.02))
          (g7 (pt 0.60 0.06)) (g8 (pt 0.70 0.03)) (g9 (pt 0.80 0.05))
          (g10 (pt 0.90 0.02)) (g11 (pt 1.00 0.04)))
      (polygon (list
                 (list ax ay) (list bx by)
                 (list (car g11) (list-ref g11 1))
                 (list (car g10) (list-ref g10 1))
                 (list (car g9)  (list-ref g9 1))
                 (list (car g8)  (list-ref g8 1))
                 (list (car g7)  (list-ref g7 1))
                 (list (car g6)  (list-ref g6 1))
                 (list (car g5)  (list-ref g5 1))
                 (list (car g4)  (list-ref g4 1))
                 (list (car g3)  (list-ref g3 1))
                 (list (car g2)  (list-ref g2 1))
                 (list (car g1)  (list-ref g1 1)))
               :fill "#3A8A2A" :stroke "none"))

    ; ── 土壤纹理: 裂纹 ──
    (let ((p1 (pt 0.10 0.50)) (p2 (pt 0.35 0.48)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#5A4028" :stroke-width 1.8))
    (let ((p1 (pt 0.30 0.60)) (p2 (pt 0.55 0.57)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#5A4028" :stroke-width 1.5))
    (let ((p1 (pt 0.50 0.72)) (p2 (pt 0.78 0.68)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#5A4028" :stroke-width 1.5))
    (let ((p1 (pt 0.68 0.85)) (p2 (pt 0.92 0.80)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#5A4028" :stroke-width 1.2))

    ; ── 土壤纹理: 卵石 ──
    (let ((p (pt 0.22 0.52)))
      (ellipse (car p) (list-ref p 1) 8 5 :fill "#9A8568" :stroke "none"))
    (let ((p (pt 0.55 0.68)))
      (ellipse (car p) (list-ref p 1) 6 4 :fill "#8A7558" :stroke "none"))
    (let ((p (pt 0.78 0.82)))
      (ellipse (car p) (list-ref p 1) 7 4 :fill "#8A7558" :stroke "none"))

    ; ── 土壤纹理: 碎石 ──
    (let ((p (pt 0.15 0.55)))
      (circle (car p) (list-ref p 1) 3 :fill "#7A6548" :stroke "none"))
    (let ((p (pt 0.42 0.63)))
      (circle (car p) (list-ref p 1) 2.5 :fill "#7A6548" :stroke "none"))
    (let ((p (pt 0.65 0.76)))
      (circle (car p) (list-ref p 1) 2 :fill "#6A5538" :stroke "none"))
    (let ((p (pt 0.85 0.88)))
      (circle (car p) (list-ref p 1) 2.5 :fill "#6A5538" :stroke "none"))
    (let ((p (pt 0.35 0.58)))
      (circle (car p) (list-ref p 1) 1.8 :fill "#6A5538" :stroke "none"))

    ; ── 岩石层纹理 ──
    (let ((p1 (pt 0.08 0.78)) (p2 (pt 0.40 0.76)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#5A4A3A" :stroke-width 1.2))
    (let ((p1 (pt 0.45 0.88)) (p2 (pt 0.85 0.85)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#5A4A3A" :stroke-width 1.0))
    (let ((p (pt 0.30 0.84)))
      (circle (car p) (list-ref p 1) 2 :fill "#7A6A58" :stroke "none"))
    (let ((p (pt 0.62 0.92)))
      (circle (car p) (list-ref p 1) 2.5 :fill "#7A6A58" :stroke "none"))))

; ═══════════════════════════════════════════════════════════════════════════════
; 右侧面 (多层土壤, 偏暗光照)
; ═══════════════════════════════════════════════════════════════════════════════

(define (right-face-soil cx cy w h d)
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
    ; ── 岩石底层 ──
    (polygon (list (list ax ay) (list bx by) (list c_x c_y) (list dx dy))
             :fill "#5A4A3A" :stroke "none" :stroke-width 0)

    ; ── 心土层 ──
    (let ((s1 (pt 0.0 0.40)) (s2 (pt 0.12 0.36)) (s3 (pt 0.25 0.41))
          (s4 (pt 0.38 0.35)) (s5 (pt 0.52 0.39)) (s6 (pt 0.65 0.34))
          (s7 (pt 0.78 0.38)) (s8 (pt 0.90 0.33)) (s9 (pt 1.0 0.36)))
      (polygon (list
                 (list (car s1) (list-ref s1 1))
                 (list (car s2) (list-ref s2 1))
                 (list (car s3) (list-ref s3 1))
                 (list (car s4) (list-ref s4 1))
                 (list (car s5) (list-ref s5 1))
                 (list (car s6) (list-ref s6 1))
                 (list (car s7) (list-ref s7 1))
                 (list (car s8) (list-ref s8 1))
                 (list (car s9) (list-ref s9 1))
                 (list bx by) (list c_x c_y) (list dx dy))
               :fill "#7A5B35" :stroke "none"))

    ; ── 腐殖土层 ──
    (let ((t1 (pt 0.0 0.40)) (t2 (pt 0.12 0.36)) (t3 (pt 0.25 0.41))
          (t4 (pt 0.38 0.35)) (t5 (pt 0.52 0.39)) (t6 (pt 0.65 0.34))
          (t7 (pt 0.78 0.38)) (t8 (pt 0.90 0.33)) (t9 (pt 1.0 0.36))
          (b1 (pt 0.0 0.10)) (b2 (pt 0.15 0.08)) (b3 (pt 0.30 0.12))
          (b4 (pt 0.45 0.07)) (b5 (pt 0.60 0.11)) (b6 (pt 0.75 0.06))
          (b7 (pt 0.90 0.09)) (b8 (pt 1.0 0.07)))
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
                 (list (car t8) (list-ref t8 1))
                 (list (car t9) (list-ref t9 1)))
               :fill "#3A2815" :stroke "none"))

    ; ── 草层覆盖 ──
    (let ((g1  (pt 0.00 0.09)) (g2  (pt 0.07 0.05)) (g3  (pt 0.14 0.11))
          (g4  (pt 0.21 0.04)) (g5  (pt 0.28 0.08)) (g6  (pt 0.35 0.03))
          (g7  (pt 0.42 0.10)) (g8  (pt 0.49 0.05)) (g9  (pt 0.56 0.09))
          (g10 (pt 0.63 0.02)) (g11 (pt 0.70 0.07)) (g12 (pt 0.77 0.04))
          (g13 (pt 0.84 0.08)) (g14 (pt 0.91 0.03)) (g15 (pt 1.00 0.06)))
      (polygon (list
                 (list ax ay) (list bx by)
                 (list (car g15) (list-ref g15 1))
                 (list (car g14) (list-ref g14 1))
                 (list (car g13) (list-ref g13 1))
                 (list (car g12) (list-ref g12 1))
                 (list (car g11) (list-ref g11 1))
                 (list (car g10) (list-ref g10 1))
                 (list (car g9)  (list-ref g9 1))
                 (list (car g8)  (list-ref g8 1))
                 (list (car g7)  (list-ref g7 1))
                 (list (car g6)  (list-ref g6 1))
                 (list (car g5)  (list-ref g5 1))
                 (list (car g4)  (list-ref g4 1))
                 (list (car g3)  (list-ref g3 1))
                 (list (car g2)  (list-ref g2 1))
                 (list (car g1)  (list-ref g1 1)))
               :fill "#225A16" :stroke "none"))

    ; ── 草层亮色叠加 ──
    (let ((g1 (pt 0.00 0.05)) (g2 (pt 0.10 0.02)) (g3 (pt 0.20 0.06))
          (g4 (pt 0.30 0.01)) (g5 (pt 0.40 0.04)) (g6 (pt 0.50 0.01))
          (g7 (pt 0.60 0.05)) (g8 (pt 0.70 0.02)) (g9 (pt 0.80 0.04))
          (g10 (pt 0.90 0.01)) (g11 (pt 1.00 0.03)))
      (polygon (list
                 (list ax ay) (list bx by)
                 (list (car g11) (list-ref g11 1))
                 (list (car g10) (list-ref g10 1))
                 (list (car g9)  (list-ref g9 1))
                 (list (car g8)  (list-ref g8 1))
                 (list (car g7)  (list-ref g7 1))
                 (list (car g6)  (list-ref g6 1))
                 (list (car g5)  (list-ref g5 1))
                 (list (car g4)  (list-ref g4 1))
                 (list (car g3)  (list-ref g3 1))
                 (list (car g2)  (list-ref g2 1))
                 (list (car g1)  (list-ref g1 1)))
               :fill "#2A6E1C" :stroke "none"))

    ; ── 土壤纹理: 裂纹 ──
    (let ((p1 (pt 0.10 0.48)) (p2 (pt 0.35 0.46)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#4A3218" :stroke-width 1.8))
    (let ((p1 (pt 0.30 0.58)) (p2 (pt 0.55 0.55)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#4A3218" :stroke-width 1.5))
    (let ((p1 (pt 0.50 0.70)) (p2 (pt 0.78 0.66)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#4A3218" :stroke-width 1.5))
    (let ((p1 (pt 0.68 0.83)) (p2 (pt 0.92 0.78)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#4A3218" :stroke-width 1.2))

    ; ── 土壤纹理: 卵石 ──
    (let ((p (pt 0.22 0.50)))
      (ellipse (car p) (list-ref p 1) 7 4 :fill "#8A7558" :stroke "none"))
    (let ((p (pt 0.55 0.66)))
      (ellipse (car p) (list-ref p 1) 5 3 :fill "#7A6548" :stroke "none"))
    (let ((p (pt 0.78 0.80)))
      (ellipse (car p) (list-ref p 1) 6 4 :fill "#7A6548" :stroke "none"))

    ; ── 土壤纹理: 碎石 ──
    (let ((p (pt 0.15 0.53)))
      (circle (car p) (list-ref p 1) 2.5 :fill "#6A5538" :stroke "none"))
    (let ((p (pt 0.42 0.61)))
      (circle (car p) (list-ref p 1) 2 :fill "#6A5538" :stroke "none"))
    (let ((p (pt 0.65 0.74)))
      (circle (car p) (list-ref p 1) 1.8 :fill "#5A4528" :stroke "none"))
    (let ((p (pt 0.85 0.86)))
      (circle (car p) (list-ref p 1) 2 :fill "#5A4528" :stroke "none"))

    ; ── 岩石层纹理 ──
    (let ((p1 (pt 0.08 0.76)) (p2 (pt 0.40 0.74)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#4A3A2A" :stroke-width 1.2))
    (let ((p1 (pt 0.45 0.86)) (p2 (pt 0.85 0.83)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#4A3A2A" :stroke-width 1.0))
    (let ((p (pt 0.30 0.82)))
      (circle (car p) (list-ref p 1) 2 :fill "#6A5A48" :stroke "none"))
    (let ((p (pt 0.62 0.90)))
      (circle (car p) (list-ref p 1) 2 :fill "#6A5A48" :stroke "none"))))

; ═══════════════════════════════════════════════════════════════════════════════
; 完整草地瓦片 (接受 tw/th/td/shader 参数)
; ═══════════════════════════════════════════════════════════════════════════════

(define (draw-grass-tile cx cy tw th td shader)
  (group
    (left-face-soil cx cy tw th td)
    (right-face-soil cx cy tw th td)
    (apply-shader! (diamond cx cy tw th) shader)))
