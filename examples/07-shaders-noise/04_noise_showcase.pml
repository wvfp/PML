;; 示例 7.4：噪声高级应用展示
;; 组合使用 Voronoi + 域扭曲 + 混合 + 量化
;; 输出: 04_noise_showcase.png

(set-backend! "skia")

;; ============================================================
;; 场景 1：Voronoi 地形
;; ============================================================
(print "生成 Voronoi 地形...")

(define v-terrain (noise-voronoi :cell-size 40 :seed 777 :jitter 0.7))
(define quantized-terrain
  (quantize-noise v-terrain
    :levels '((0.25 "#2d5a27")   ;; 深绿森林
              (0.50 "#6b8e23")   ;; 浅绿草地
              (0.75 "#a0895c")   ;; 黄褐荒漠
              (1.0  "#8b7355")))) ;; 棕色山岩

(canvas 256 256)
(add (apply-shader! (rect 0 0 256 256 :fill "white") quantized-terrain))
(render "04_noise_showcase_terrain.png")

;; ============================================================
;; 场景 2：扭曲大理石纹理
;; ============================================================
(print "生成扭曲大理石纹理...")

(define marble-base (noise-turbulence :octaves 6 :scale 0.02 :seed 13))
(define marble-warp (noise-fractal :octaves 2 :scale 0.005 :seed 55))

(define marble (noise-warp marble-base marble-warp :amount 40.0 :freq 0.006))
(define quantized-marble
  (quantize-noise marble
    :levels '((0.2 "#f5f0eb")    ;; 亮白
              (0.4 "#d4c9b8")    ;; 浅米
              (0.6 "#c4b49a")    ;; 中米
              (0.8 "#8b7355")    ;; 棕灰
              (1.0 "#4a3728")))) ;; 深棕

(canvas 256 256)
(add (apply-shader! (rect 0 0 256 256 :fill "white") quantized-marble))
(render "04_noise_showcase_marble.png")

;; ============================================================
;; 场景 3：纹理过渡（Voronoi → 扭曲大理石 渐变混合）
;; ============================================================
(print "生成纹理过渡...")

(define transition
  (noise-blend v-terrain quantized-marble :mode 'gradient :weight 0.5))

(canvas 256 256)
(add (apply-shader! (rect 0 0 256 256 :fill "white") transition))
(render "04_noise_showcase_transition.png")

;; ============================================================
;; 场景 4：分割对比 — 四种噪声效果同屏
;; ============================================================
(print "生成分割对比图...")

(canvas 512 512 :bg "#1a1a2e")

;; 左上：原始 Voronoi 地形
(add (with-transform
       (apply-shader! (rect 0 0 256 256 :fill "white") quantized-terrain)
       (translate 0 0)))

;; 右上：扭曲大理石
(add (with-transform
       (apply-shader! (rect 0 0 256 256 :fill "white") quantized-marble)
       (translate 256 0)))

;; 左下：域扭曲 + Voronoi（用 Voronoi 做 warp field 扭曲分形噪声）
(define show-warp (noise-warp marble-base v-terrain :amount 30.0 :freq 0.01))
(add (with-transform
       (apply-shader! (rect 0 0 256 256 :fill "white") show-warp)
       (translate 0 256)))

;; 右下：三路混合（Voronoi + marble + warp）
(define triple-a (noise-blend v-terrain marble :mode 'horizontal :weight 0.5))
(define triple  (noise-blend triple-a show-warp :mode 'vertical :weight 0.5))
(add (with-transform
       (apply-shader! (rect 0 0 256 256 :fill "white") triple)
       (translate 256 256)))

(render "04_noise_showcase.png")

(print "✓ 所有高级噪声展示已渲染完成 → 07-shaders-noise/")
