; stone_cube_lib.pml — 石头正方体瓦片库
; 全手绘岩石纹理，与草地侧面岩石层无缝拼接

(provide draw-stone-cube)

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
; 顶面 — 手绘岩石
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
             :fill "#4A4540" :stroke "none" :stroke-width 0)

    ; 裂纹
    (let ((p1 (tp 0.25 0.38)) (p2 (tp 0.48 0.35)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#1A1815" :stroke-width 1.5))
    (let ((p1 (tp 0.45 0.55)) (p2 (tp 0.68 0.52)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#1A1815" :stroke-width 1.2))

    ; 层理线
    (let ((p1 (tp 0.30 0.48)) (p2 (tp 0.60 0.45)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#6A6560" :stroke-width 0.8))

    ; 矿物斑点
    (let ((p (tp 0.38 0.42)))
      (circle (car p) (list-ref p 1) 3 :fill "#6A6560" :stroke "none"))
    (let ((p (tp 0.58 0.52)))
      (circle (car p) (list-ref p 1) 2.5 :fill "#5A5550" :stroke "none"))
    (let ((p (tp 0.48 0.60)))
      (circle (car p) (list-ref p 1) 2 :fill "#5A5550" :stroke "none"))))

; ═══════════════════════════════════════════════════════════════════════════════
; 左侧面 — 统一岩石 (与草地侧面岩石层衔接)
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
    ; ── 统一基色 ──
    (polygon (list (list ax ay) (list bx by) (list c_x c_y) (list dx dy))
             :fill "#3A3530" :stroke "none" :stroke-width 0)

    ; ── 岩石裂纹 ──
    (let ((p1 (pt 0.08 0.15)) (p2 (pt 0.32 0.13)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#1A1815" :stroke-width 1.5))
    (let ((p1 (pt 0.30 0.35)) (p2 (pt 0.60 0.32)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#1A1815" :stroke-width 1.2))
    (let ((p1 (pt 0.50 0.55)) (p2 (pt 0.82 0.52)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#1A1815" :stroke-width 1.0))

    ; ── 层理线 ──
    (let ((p1 (pt 0.05 0.42)) (p2 (pt 0.45 0.40)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#5A5550" :stroke-width 0.8))
    (let ((p1 (pt 0.50 0.68)) (p2 (pt 0.92 0.66)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#5A5550" :stroke-width 0.8))

    ; ── 矿物斑点 ──
    (let ((p (pt 0.22 0.25)))
      (circle (car p) (list-ref p 1) 3 :fill "#6A6560" :stroke "none"))
    (let ((p (pt 0.55 0.45)))
      (circle (car p) (list-ref p 1) 2.5 :fill "#5A5550" :stroke "none"))
    (let ((p (pt 0.78 0.62)))
      (circle (car p) (list-ref p 1) 2 :fill "#5A5550" :stroke "none"))))

; ═══════════════════════════════════════════════════════════════════════════════
; 右侧面 — 统一岩石 (偏暗)
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
    ; ── 统一基色 ──
    (polygon (list (list ax ay) (list bx by) (list c_x c_y) (list dx dy))
             :fill "#2A2520" :stroke "none" :stroke-width 0)

    ; ── 岩石裂纹 ──
    (let ((p1 (pt 0.08 0.13)) (p2 (pt 0.32 0.11)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#0E0C0A" :stroke-width 1.5))
    (let ((p1 (pt 0.30 0.33)) (p2 (pt 0.60 0.30)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#0E0C0A" :stroke-width 1.2))
    (let ((p1 (pt 0.50 0.53)) (p2 (pt 0.82 0.50)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#0E0C0A" :stroke-width 1.0))

    ; ── 层理线 ──
    (let ((p1 (pt 0.05 0.40)) (p2 (pt 0.45 0.38)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#4A4540" :stroke-width 0.8))
    (let ((p1 (pt 0.50 0.66)) (p2 (pt 0.92 0.64)))
      (line (car p1) (list-ref p1 1) (car p2) (list-ref p2 1)
            :stroke "#4A4540" :stroke-width 0.8))

    ; ── 矿物斑点 ──
    (let ((p (pt 0.22 0.23)))
      (circle (car p) (list-ref p 1) 2.5 :fill "#5A5550" :stroke "none"))
    (let ((p (pt 0.55 0.43)))
      (circle (car p) (list-ref p 1) 2 :fill "#4A4540" :stroke "none"))
    (let ((p (pt 0.78 0.60)))
      (circle (car p) (list-ref p 1) 1.8 :fill "#4A4540" :stroke "none"))))

; ═══════════════════════════════════════════════════════════════════════════════
; 完整岩石正方体
; ═══════════════════════════════════════════════════════════════════════════════

(define (draw-stone-cube cx cy tw th td)
  (group
    (left-face cx cy tw th td)
    (right-face cx cy tw th td)
    (top-face cx cy tw th)))
