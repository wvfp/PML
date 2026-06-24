; ═══════════════════════════════════════════════════════════════════════════════
; 示例 6：有机 Blob — 贝塞尔曲线构成的有机形状 + 纹理
; ───────────────────────────────────────────────────────────────────────────────
; 展示如何使用 path 绘制复杂的有机形状，并结合噪声纹理着色器。
;
; 运行：pml 06_organic_blob_path_texture.pml
; ═══════════════════════════════════════════════════════════════════════════════

(set-backend! "skia")
(canvas 512 512 :bg "#0d0d1a")

; ── Step 1: 创建纹理底图（噪点）───────────────────────────────────────────
(define tex-shader (noise-fractal 512 512 :freq-x 0.015 :freq-y 0.015 :octaves 4 :seed 777))

; ── Step 2: 用贝塞尔曲线构建有机形状 ─────────────────────────────────────
(define blob
  (path :d (string-append
    "M 180 80 "
    "C 220 40, 300 30, 350 70 "
    "C 400 110, 450 90, 470 140 "
    "C 490 190, 480 250, 440 280 "
    "C 420 300, 430 340, 400 380 "
    "C 370 420, 320 400, 290 430 "
    "C 250 460, 200 440, 170 410 "
    "C 130 380, 100 400, 80 350 "
    "C 60 300, 90 250, 70 200 "
    "C 50 150, 100 120, 180 80 Z")
    :fill "#2d5a27"
    :stroke "#4a9b3f"
    :stroke-width 3
    :roughness 2.0
    :bowing 0.8))

(define textured-blob (apply-shader! blob tex-shader))
(add textured-blob)

; ── Step 3: 内部装饰性流线 ───────────────────────────────────────────────
(define line-1
  (path :d "M 120 200 C 180 160, 250 150, 320 180 C 370 200, 410 220, 430 260"
        :stroke "#5cb85c" :stroke-width 1.5 :roughness 1.0))
(define line-2
  (path :d "M 100 300 C 160 280, 220 310, 280 280 C 330 250, 380 270, 420 290"
        :stroke "#5cb85c" :stroke-width 1 :roughness 0.8))
(define line-3
  (path :d "M 140 350 C 190 370, 240 350, 290 380 C 340 410, 370 390, 400 360"
        :stroke "#5cb85c" :stroke-width 1.2 :roughness 1.2))

(add line-1)
(add line-2)
(add line-3)

; ── Step 4: 周围散布粒子（使用 path 命令列表绘制圆形）────────────────────
(define dot (lambda (cx cy r clr)
  (path (list (list 'move-to cx (- cy r))
              (list 'cubic-to (+ cx (* r 0.55)) (- cy r) (+ cx r) (- cy (* r 0.55)) (+ cx r) cy)
              (list 'cubic-to (+ cx r) (+ cy (* r 0.55)) (+ cx (* r 0.55)) (+ cy r) cx (+ cy r))
              (list 'cubic-to (- cx (* r 0.55)) (+ cy r) (- cx r) (+ cy (* r 0.55)) (- cx r) cy)
              (list 'cubic-to (- cx r) (- cy (* r 0.55)) (- cx (* r 0.55)) (- cy r) cx (- cy r))
              (list 'close))
        :fill clr)))

(add (dot 60 120 4 "#6bcf6b"))
(add (dot 440 60 3 "#6bcf6b"))
(add (dot 470 330 5 "#6bcf6b"))
(add (dot 50 300 3 "#5cb85c"))
(add (dot 380 440 4 "#5cb85c"))
(add (dot 200 50 3 "#4a9b3f"))
(add (dot 90 450 3 "#6bcf6b"))
(add (dot 430 180 4 "#5cb85c"))

(render "06_organic_blob_path_texture.png")
