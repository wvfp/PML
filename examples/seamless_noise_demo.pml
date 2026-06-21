;; ============================================================
;; seamless_noise_demo.pml
;; 演示无缝噪声（noise-fractal / noise-turbulence）效果
;; 用法: pml.exe seamless_noise_demo.pml
;; 输出: examples/output/ 下 8 张 PNG
;; ============================================================

(define width 512)
(define height 512)
(define tile-size 128)

;; ── 1. 基本分形噪声（无平铺，作为对比参考） ─────────────
(canvas width height)
(define n1 (noise-fractal :seed 42 :octaves 6 :freq-x 0.03 :freq-y 0.03))
(add (apply-shader! (rect 0 0 width height :fill "white") n1))
(render "output/noise_fractal_basic.png")

;; ── 2. 无缝分形噪声（128×128 tile 拼满 512×512） ─────────
(canvas width height)
(define n2 (noise-fractal :seed 42 :octaves 6 :freq-x 0.03 :freq-y 0.03
                          :tile-width tile-size :tile-height tile-size))
(for-each (lambda (row)
            (for-each (lambda (col)
                        (define tile (apply-shader!
                                      (rect 0 0 tile-size tile-size :fill "white")
                                      n2))
                        (add (with-transform tile
                              (translate (* col tile-size) (* row tile-size)))))
                      (range 0 4)))
          (range 0 4))
(render "output/noise_fractal_seamless.png")

;; ── 3. 无缝湍流噪声 ───────────────────────────────────────
(canvas width height)
(define n3 (noise-turbulence :seed 99 :octaves 5 :freq-x 0.04 :freq-y 0.04
                             :tile-width tile-size :tile-height tile-size))
(for-each (lambda (row)
            (for-each (lambda (col)
                        (define tile (apply-shader!
                                      (rect 0 0 tile-size tile-size :fill "white")
                                      n3))
                        (add (with-transform tile
                              (translate (* col tile-size) (* row tile-size)))))
                      (range 0 4)))
          (range 0 4))
(render "output/noise_turbulence_seamless.png")

;; ── 4. 色阶量化 — 四色石板 tile ────────────────────────────
(canvas 256 256)
(define n4 (noise-fractal :seed 42 :octaves 6
                          :freq-x 0.04 :freq-y 0.04
                          :tile-width 64 :tile-height 64))
(define stone (quantize-noise n4
  :levels '((0.25 "#2a2a2a")
            (0.50 "#555555")
            (0.75 "#8a8a8a")
            (1.0  "#bbbbbb"))))
(for-each (lambda (row)
            (for-each (lambda (col)
                        (define tile (apply-shader!
                                      (rect 0 0 64 64 :fill "white")
                                      stone))
                        (add (with-transform tile
                              (translate (* col 64) (* row 64)))))
                      (range 0 4)))
          (range 0 4))
(render "output/noise_stone_tiles.png")

;; ── 5. 色阶量化 — 三色地形 tile ────────────────────────────
(canvas 256 256)
(define n5 (noise-turbulence :seed 99 :octaves 4
                             :freq-x 0.06 :freq-y 0.06
                             :tile-width 64 :tile-height 64))
(define terrain (quantize-noise n5
  :levels '((0.33 "#1a3a5c")
            (0.66 "#4a8c3f")
            (1.0  "#d4b483"))))
(for-each (lambda (row)
            (for-each (lambda (col)
                        (define tile (apply-shader!
                                      (rect 0 0 64 64 :fill "white")
                                      terrain))
                        (add (with-transform tile
                              (translate (* col 64) (* row 64)))))
                      (range 0 4)))
          (range 0 4))
(render "output/noise_terrain_tiles.png")

;; ── 6. 对比：同一种子，有/无平铺 ──────────────────────────
;; 左半边：无平铺（可以看到边界接缝）
;; 右半边：128×128 无缝平铺（看不到接缝）
(canvas 512 256)
(define n6a (noise-fractal :seed 7 :octaves 4 :freq-x 0.05 :freq-y 0.05))
(add (apply-shader! (rect 0 0 256 256 :fill "white") n6a))
(define n6b (noise-fractal :seed 7 :octaves 4 :freq-x 0.05 :freq-y 0.05
                           :tile-width 128 :tile-height 128))
(for-each (lambda (row)
            (for-each (lambda (col)
                        (define tile (apply-shader!
                                      (rect 0 0 128 128 :fill "white")
                                      n6b))
                        (add (with-transform tile
                              (translate (+ 256 (* col 128))
                                         (* row 128)))))
                      (range 0 2)))
          (range 0 2))
(render "output/noise_seamless_vs_none.png")

;; ── 7. 高 octaves 细节噪声（无缝） ────────────────────────
(canvas width height)
(define n7 (noise-fractal :seed 123 :octaves 8 :freq-x 0.08 :freq-y 0.08
                          :tile-width tile-size :tile-height tile-size))
(for-each (lambda (row)
            (for-each (lambda (col)
                        (define tile (apply-shader!
                                      (rect 0 0 tile-size tile-size :fill "white")
                                      n7))
                        (add (with-transform tile
                              (translate (* col tile-size) (* row tile-size)))))
                      (range 0 4)))
          (range 0 4))
(render "output/noise_fractal_detail.png")

;; ── 8. 色阶量化 — 云层效果（蓝白配色） ────────────────────
(canvas 256 256)
(define n8 (noise-fractal :seed 200 :octaves 5
                          :freq-x 0.03 :freq-y 0.03
                          :tile-width 128 :tile-height 128))
(define clouds (quantize-noise n8
  :levels '((0.30 "#0a1628")
            (0.55 "#1a3a6c")
            (0.78 "#4a7ab5")
            (0.90 "#8ab4f8")
            (1.0  "#dceaff"))))
(for-each (lambda (row)
            (for-each (lambda (col)
                        (define tile (apply-shader!
                                      (rect 0 0 128 128 :fill "white")
                                      clouds))
                        (add (with-transform tile
                              (translate (* col 128) (* row 128)))))
                      (range 0 2)))
          (range 0 2))
(render "output/noise_clouds.png")

(print "✓ 所有无缝噪声示例已渲染完成 → examples/output/")
