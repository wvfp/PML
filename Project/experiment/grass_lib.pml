; grass_lib.pml — 草地瓦片共享库
; 提供：动漫草地 shader、多层土壤侧面、draw-grass-tile

(provide grass-shader draw-grass-tile draw-edge-tile)

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
; 顶面菱形（底部圆角）
; ═══════════════════════════════════════════════════════════════════════════════

(define (diamond cx cy w h)
  (define r 20)
  (define top-y (- cy (/ h 2)))
  (define left-x (- cx (/ w 2)))
  (define right-x (+ cx (/ w 2)))
  (define bottom-y (+ cy (/ h 2)))
  
  (define side-angle 0.6435)
  (define offset-x (* r (cos side-angle)))
  (define offset-y (* r (sin side-angle)))
  
  (define pt-right-x (- right-x offset-x))
  (define pt-right-y (+ cy offset-y))
  (define pt-left-x (+ left-x offset-x))
  (define pt-left-y (+ cy offset-y))
  
  (define arc-center-x cx)
  (define arc-center-y (+ bottom-y r))
  
  (define (pt-on-arc center-x center-y radius start-angle end-angle steps n)
    (define t (/ n steps))
    (define angle (+ start-angle (* t (- end-angle start-angle))))
    (list (+ center-x (* radius (cos angle)))
          (+ center-y (* radius (sin angle)))))
  
  (define bottom-arc-pts '())
  (do ((i 0 (+ i 1)))
      ((> i 8))
    (set! bottom-arc-pts (cons (pt-on-arc arc-center-x arc-center-y r 0 3.14159265 8 i) bottom-arc-pts)))
  
  (polygon (append (list (list cx top-y)
                         (list right-x cy)
                         (list pt-right-x pt-right-y))
                   bottom-arc-pts
                   (list (list pt-left-x pt-left-y)
                         (list left-x cy)))
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

; ═══════════════════════════════════════════════════════════════════════════════
; 边界瓦片: 用多边形扰动形成不规则场景边界
; ═══════════════════════════════════════════════════════════════════════════════

(define (lerp a b t)
  (+ a (* t (- b a))))

(define (pseudo-random i seed)
  (define x (+ i (* seed 0.713)))
  (define y (* x 12.9898))
  (define z (+ (* y 78.233) (* seed 437.581)))
  (- (* 2 (abs (sin z))) 1.0))

; 生成从 p1 到 p2 的圆润外凸边（含端点）
; base-dx/base-dy 为最大外凸方向，amp 保留以兼容接口
(define (perturb-edge p1 p2 n base-dx base-dy amp seed)
  (define x1 (car p1))
  (define y1 (list-ref p1 1))
  (define x2 (car p2))
  (define y2 (list-ref p2 1))
  (define pts (list p1))
  (do ((i 1 (+ i 1)))
      ((> i n) (append (reverse pts) (list p2)))
    (define t (/ i (+ n 1)))
    (define bx (lerp x1 x2 t))
    (define by (lerp y1 y2 t))
    ; 低频圆润波浪：只有 1~2 个大起伏，始终外凸
    (define wave (+ (* 0.70 (sin (+ (* t 1.9) seed)))
                    (* 0.30 (sin (+ (* t 3.7) (* seed 1.6))))))
    ; 端点处偏移为 0，中段圆润饱满，保证相邻 tile 角点接合
    (define envelope (sqrt (sin (* 3.141592653589793 t))))
    (define r (* envelope (+ 0.25 (* 0.75 (abs wave)))))
    (define ox (* base-dx r))
    (define oy (* base-dy r))
    (set! pts (cons (list (+ bx ox) (+ by oy)) pts))))

; 返回 (list top-pts outer-edges)
; top-pts: 顶面多边形顶点
; outer-edges: 外侧边列表，每条边是包含端点的点列
(define (edge-tile-top cx cy tw th edge-type amplitude seed)
  (define W (list (- cx (/ tw 2)) cy))
  (define N (list cx (- cy (/ th 2))))
  (define E (list (+ cx (/ tw 2)) cy))
  (define S (list cx (+ cy (/ th 2))))
  (define a amplitude)
  (define a2 (/ a 2))

  (define (pe p1 p2 n dx dy)
    (perturb-edge p1 p2 n dx dy a seed))

  (cond
    ((eq? edge-type 'center) (list (list N E S W) '()))
    ; 外凸方向沿场景外侧法线（y 向下增长，故 top 向上为负）
    ((eq? edge-type 'top)
     (define wn (pe W N 8 0 (- a)))
     (define ne (pe N E 8 0 (- a)))
     (list (append wn (cdr ne) (list S))
           (list wn ne)))
    ((eq? edge-type 'bottom)
     (define es (pe E S 8 0 a))
     (define sw (pe S W 8 0 a))
     (list (append (list N) es (cdr sw))
           (list es sw)))
    ((eq? edge-type 'left)
     (define sw (pe S W 8 (- a) 0))
     (define wn (pe W N 8 (- a) 0))
     (list (append (list E) sw (cdr wn))
           (list sw wn)))
    ((eq? edge-type 'right)
     (define ne (pe N E 8 a 0))
     (define es (pe E S 8 a 0))
     (list (append ne (cdr es) (list W))
           (list ne es)))
    ((eq? edge-type 'top-left)
     (define en (pe E N 6 (- a) (- a)))
     (define nw (pe N W 6 (- a) (- a)))
     (list (append en (cdr nw) (list S))
           (list en nw)))
    ((eq? edge-type 'top-right)
     (define wn (pe W N 6 a (- a)))
     (define ne (pe N E 6 a (- a)))
     (list (append wn (cdr ne) (list S))
           (list wn ne)))
    ((eq? edge-type 'bottom-left)
     (define es (pe E S 6 (- a) a))
     (define sw (pe S W 6 (- a) a))
     (list (append (list N) es (cdr sw))
           (list es sw)))
    ((eq? edge-type 'bottom-right)
     (define ne (pe N E 6 a a))
     (define es (pe E S 6 a a))
     (list (append ne (cdr es) (list W))
           (list ne es)))))

; 挤压一条外侧边（点列）形成带纹理的连续侧面
(define (extrude-edge-soil edge-pts depth base-color)
  (define n (length edge-pts))
  (define top-rev '())
  (define bottom '())
  (do ((i 0 (+ i 1)))
      ((= i n))
    (define p (list-ref edge-pts i))
    (define b (list (car p) (+ (list-ref p 1) depth)))
    (set! top-rev (cons p top-rev))
    (set! bottom (cons b bottom)))
  (define side-pts (append top-rev (reverse bottom)))

  ; 侧面内部纹理辅助：沿边取点，按深度比例插值
  (define (side-pt i ratio)
    (define p (list-ref edge-pts i))
    (list (car p)
          (+ (list-ref p 1) (* depth ratio))))

  (group
    ; ── 岩石底层 ──
    (polygon side-pts :fill base-color :stroke "none" :stroke-width 0)

    ; ── 心土层（约 0.35~0.55 深度）──
    (let ((spans '()))
      (do ((i 0 (+ i 1)))
          ((= i (- n 1)))
        (define s1 (side-pt i 0.35))
        (define s2 (side-pt (+ i 1) 0.35))
        (define s3 (side-pt (+ i 1) 0.55))
        (define s4 (side-pt i 0.55))
        (set! spans (cons (polygon (list s1 s2 s3 s4)
                                   :fill "#7A6548" :stroke "none") spans)))
      (apply group spans))

    ; ── 腐殖土层（约 0.12~0.32 深度）──
    (let ((spans '()))
      (do ((i 0 (+ i 1)))
          ((= i (- n 1)))
        (define s1 (side-pt i 0.12))
        (define s2 (side-pt (+ i 1) 0.12))
        (define s3 (side-pt (+ i 1) 0.32))
        (define s4 (side-pt i 0.32))
        (set! spans (cons (polygon (list s1 s2 s3 s4)
                                   :fill "#4A3520" :stroke "none") spans)))
      (apply group spans))

    ; ── 草层（约 0.02~0.12 深度）──
    (let ((spans '()))
      (do ((i 0 (+ i 1)))
          ((= i (- n 1)))
        (define s1 (side-pt i 0.02))
        (define s2 (side-pt (+ i 1) 0.02))
        (define s3 (side-pt (+ i 1) 0.12))
        (define s4 (side-pt i 0.12))
        (set! spans (cons (polygon (list s1 s2 s3 s4)
                                   :fill "#2D5A1A" :stroke "none") spans)))
      (apply group spans))

    ; ── 土壤裂纹 ──
    (let* ((i1 (quotient n 3))
           (i2 (quotient (* n 2) 3))
           (p1 (side-pt i1 0.45))
           (p2 (side-pt (+ i1 2) 0.42))
           (p3 (side-pt i2 0.65))
           (p4 (side-pt (+ i2 2) 0.62)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#5A4028" :stroke-width 1.5)
      (line (car p3) (list-ref p3 1) (car p4) (list-ref p4 1)
            :stroke "#5A4028" :stroke-width 1.2))

    ; ── 卵石 ──
    (let ((p1 (side-pt (quotient n 4) 0.50))
          (p2 (side-pt (quotient (* n 3) 4) 0.72)))
      (ellipse (car p1) (list-ref p1 1) 7 4
               :fill "#9A8568" :stroke "none")
      (ellipse (car p2) (list-ref p2 1) 5 3
               :fill "#8A7558" :stroke "none"))

    ; ── 碎石 ──
    (let ((p1 (side-pt (quotient n 5) 0.58))
          (p2 (side-pt (quotient (* n 2) 5) 0.78))
          (p3 (side-pt (quotient (* n 3) 5) 0.85)))
      (circle (car p1) (list-ref p1 1) 2.5 :fill "#7A6548" :stroke "none")
      (circle (car p2) (list-ref p2 1) 2   :fill "#6A5538" :stroke "none")
      (circle (car p3) (list-ref p3 1) 1.8 :fill "#6A5538" :stroke "none"))))

(define (draw-edge-tile cx cy tw th td shader edge-type amplitude seed)
  (define result (edge-tile-top cx cy tw th edge-type amplitude seed))
  (define top-pts (car result))
  (define outer-edges (list-ref result 1))
  (define sides '())
  (do ((i 0 (+ i 1)))
      ((= i (length outer-edges))
       (group
         (apply group (reverse sides))
         (apply-shader! (polygon top-pts :fill "#3D8C2E" :stroke "#2D6B1E" :stroke-width 1.5) shader)))
    (set! sides (cons (extrude-edge-soil (list-ref outer-edges i) td "#6B5B4A") sides))))
