; dirt_cube_lib.pml — 泥土正方体瓦片库
; 全手绘泥土纹理，与 grass_lib 侧面土壤无缝拼接

(provide draw-dirt-cube)

; ═══════════════════════════════════════════════════════════════════════════════
; 面内插值 (u=0~1, v=0~1)
; ═══════════════════════════════════════════════════════════════════════════════

(define (fp ax ay bx by dx dy cx_ cy_ u v)
  (define t_x (+ ax (* u (- bx ax))))
  (define t_y (+ ay (* u (- by ay))))
  (define b_x (+ dx (* u (- cx_ dx))))
  (define b_y (+ dy (* u (- cy_ dy))))
  (list (+ t_x (* v (- b_x t_x)))
        (+ t_y (* v (- b_y t_y)))))

; ═══════════════════════════════════════════════════════════════════════════════
; 顶面 — 手绘泥土 (与侧面一致)
; ═══════════════════════════════════════════════════════════════════════════════

(define (top-face cx cy w h)
  (define top-y (- cy (/ h 2)))
  (define bot-y (+ cy (/ h 2)))
  (define lft-x (- cx (/ w 2)))
  (define rgt-x (+ cx (/ w 2)))
  ; 菱形四角: A=顶, B=右, D=左, C=底
  (define (tp u v)
    (fp cx top-y rgt-x cy lft-x cy cx bot-y u v))

  (group
    ; 基色
    (polygon (list
               (list cx top-y) (list rgt-x cy)
               (list cx bot-y) (list lft-x cy))
             :fill "#6B5040" :stroke "none" :stroke-width 0)

    ; 裂纹
    (let ((p1 (tp 0.25 0.35)) (p2 (tp 0.50 0.40)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#3A2815" :stroke-width 1.5))
    (let ((p1 (tp 0.40 0.55)) (p2 (tp 0.65 0.50)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#3A2815" :stroke-width 1.2))
    (let ((p1 (tp 0.55 0.65)) (p2 (tp 0.75 0.60)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#3A2815" :stroke-width 1.0))

    ; 卵石
    (let ((p (tp 0.35 0.45)))
      (ellipse (car p) (list-ref p 1) 6 4 :fill "#9A8568" :stroke "none"))
    (let ((p (tp 0.60 0.55)))
      (ellipse (car p) (list-ref p 1) 5 3 :fill "#8A7558" :stroke "none"))

    ; 碎石
    (let ((p (tp 0.30 0.50)))
      (circle (car p) (list-ref p 1) 2.5 :fill "#7A6548" :stroke "none"))
    (let ((p (tp 0.50 0.48)))
      (circle (car p) (list-ref p 1) 2 :fill "#6A5538" :stroke "none"))
    (let ((p (tp 0.68 0.58)))
      (circle (car p) (list-ref p 1) 2 :fill "#6A5538" :stroke "none"))
    (let ((p (tp 0.45 0.62)))
      (circle (car p) (list-ref p 1) 1.8 :fill "#6A5538" :stroke "none"))))

; ═══════════════════════════════════════════════════════════════════════════════
; 左侧面 — 统一泥土 (与草地侧面土壤衔接)
; ═══════════════════════════════════════════════════════════════════════════════

(define (left-face cx cy w h d)
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
    ; ── 统一基色 (心土) ──
    (polygon (list (list ax ay) (list bx by) (list c_x c_y) (list dx dy))
             :fill "#8B6B45" :stroke "none" :stroke-width 0)

    ; ── 裂纹 ──
    (let ((p1 (pt 0.08 0.15)) (p2 (pt 0.35 0.13)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#4A3A28" :stroke-width 1.8))
    (let ((p1 (pt 0.25 0.32)) (p2 (pt 0.55 0.29)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#4A3A28" :stroke-width 1.5))
    (let ((p1 (pt 0.45 0.50)) (p2 (pt 0.78 0.47)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#4A3A28" :stroke-width 1.5))
    (let ((p1 (pt 0.60 0.68)) (p2 (pt 0.92 0.64)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#4A3A28" :stroke-width 1.2))

    ; ── 卵石 ──
    (let ((p (pt 0.18 0.22)))
      (ellipse (car p) (list-ref p 1) 8 5 :fill "#9A8568" :stroke "none"))
    (let ((p (pt 0.50 0.42)))
      (ellipse (car p) (list-ref p 1) 6 4 :fill "#8A7558" :stroke "none"))
    (let ((p (pt 0.75 0.60)))
      (ellipse (car p) (list-ref p 1) 7 4 :fill "#8A7558" :stroke "none"))

    ; ── 碎石 ──
    (let ((p (pt 0.12 0.38)))
      (circle (car p) (list-ref p 1) 3 :fill "#7A6548" :stroke "none"))
    (let ((p (pt 0.38 0.55)))
      (circle (car p) (list-ref p 1) 2.5 :fill "#7A6548" :stroke "none"))
    (let ((p (pt 0.62 0.72)))
      (circle (car p) (list-ref p 1) 2 :fill "#5A4A3A" :stroke "none"))
    (let ((p (pt 0.82 0.80)))
      (circle (car p) (list-ref p 1) 2.5 :fill "#5A4A3A" :stroke "none"))
    (let ((p (pt 0.30 0.48)))
      (circle (car p) (list-ref p 1) 1.8 :fill "#5A4A3A" :stroke "none"))

    ; ── 层理线 ──
    (let ((p1 (pt 0.05 0.58)) (p2 (pt 0.40 0.56)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#5A4A3A" :stroke-width 1.2))
    (let ((p1 (pt 0.45 0.78)) (p2 (pt 0.85 0.75)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#5A4A3A" :stroke-width 1.0))
    (let ((p (pt 0.28 0.70)))
      (circle (car p) (list-ref p 1) 2 :fill "#7A6A58" :stroke "none"))
    (let ((p (pt 0.65 0.88)))
      (circle (car p) (list-ref p 1) 2.5 :fill "#7A6A58" :stroke "none"))))

; ═══════════════════════════════════════════════════════════════════════════════
; 右侧面 — 统一泥土 (偏暗)
; ═══════════════════════════════════════════════════════════════════════════════

(define (right-face cx cy w h d)
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
    ; ── 统一基色 (心土偏暗) ──
    (polygon (list (list ax ay) (list bx by) (list c_x c_y) (list dx dy))
             :fill "#7A5B35" :stroke "none" :stroke-width 0)

    ; ── 裂纹 ──
    (let ((p1 (pt 0.08 0.13)) (p2 (pt 0.35 0.11)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#3A2A18" :stroke-width 1.8))
    (let ((p1 (pt 0.25 0.30)) (p2 (pt 0.55 0.27)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#3A2A18" :stroke-width 1.5))
    (let ((p1 (pt 0.45 0.48)) (p2 (pt 0.78 0.45)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#3A2A18" :stroke-width 1.5))
    (let ((p1 (pt 0.60 0.66)) (p2 (pt 0.92 0.62)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#3A2A18" :stroke-width 1.2))

    ; ── 卵石 ──
    (let ((p (pt 0.18 0.20)))
      (ellipse (car p) (list-ref p 1) 7 4 :fill "#8A7558" :stroke "none"))
    (let ((p (pt 0.50 0.40)))
      (ellipse (car p) (list-ref p 1) 5 3 :fill "#7A6548" :stroke "none"))
    (let ((p (pt 0.75 0.58)))
      (ellipse (car p) (list-ref p 1) 6 4 :fill "#7A6548" :stroke "none"))

    ; ── 碎石 ──
    (let ((p (pt 0.12 0.36)))
      (circle (car p) (list-ref p 1) 2.5 :fill "#6A5538" :stroke "none"))
    (let ((p (pt 0.38 0.53)))
      (circle (car p) (list-ref p 1) 2 :fill "#6A5538" :stroke "none"))
    (let ((p (pt 0.62 0.70)))
      (circle (car p) (list-ref p 1) 1.8 :fill "#4A3A2A" :stroke "none"))
    (let ((p (pt 0.82 0.78)))
      (circle (car p) (list-ref p 1) 2 :fill "#4A3A2A" :stroke "none"))

    ; ── 层理线 ──
    (let ((p1 (pt 0.05 0.56)) (p2 (pt 0.40 0.54)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#4A3A2A" :stroke-width 1.2))
    (let ((p1 (pt 0.45 0.76)) (p2 (pt 0.85 0.73)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#4A3A2A" :stroke-width 1.0))
    (let ((p (pt 0.28 0.68)))
      (circle (car p) (list-ref p 1) 2 :fill "#6A5A48" :stroke "none"))
    (let ((p (pt 0.65 0.86)))
      (circle (car p) (list-ref p 1) 2 :fill "#6A5A48" :stroke "none"))))

; ═══════════════════════════════════════════════════════════════════════════════
; 完整泥土正方体
; ═══════════════════════════════════════════════════════════════════════════════

(define (draw-dirt-cube cx cy tw th td)
  (group
    (left-face cx cy tw th td)
    (right-face cx cy tw th td)
    (top-face cx cy tw th)))
