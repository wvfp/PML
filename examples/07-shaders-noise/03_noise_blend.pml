;; 示例 7.3：噪声混合（Noise Blend）
;; 展示 noise-blend 三种过渡模式
;; 输出: 03_noise_blend.png

(set-backend! "skia")

;; ---- 准备两种不同类型的噪声 ----------------------------------------------------------------─
(define noise-a
  (noise-voronoi :cell-size 24 :seed 0 :jitter 0.5))

(define noise-b
  (noise-fractal :octaves 5 :scale 0.03 :seed 42))

;; ---- 生成三种混合模式 ------------------------------------------------------------------------─

;; 渐变模式（径向过渡）
(define grad-mix (noise-blend noise-a noise-b :mode 'gradient :weight 0.5))
(canvas 256 256)
(add (apply-shader! (rect 0 0 256 256 :fill "white") grad-mix))
(render "03_noise_blend_gradient.png")

;; 水平模式（从左到右过渡）
(define horiz-mix (noise-blend noise-a noise-b :mode 'horizontal :weight 0.4))
(canvas 256 256)
(add (apply-shader! (rect 0 0 256 256 :fill "white") horiz-mix))
(render "03_noise_blend_horizontal.png")

;; 垂直模式（从上到下过渡）
(define vert-mix (noise-blend noise-a noise-b :mode 'vertical :weight 0.6))
(canvas 256 256)
(add (apply-shader! (rect 0 0 256 256 :fill "white") vert-mix))
(render "03_noise_blend_vertical.png")

;; ---- 三图并排对比 --------------------------------------------------------------------------------─
(canvas 768 256 :bg "#1a1a2e")

(add (with-transform
       (apply-shader! (rect 0 0 256 256 :fill "white") grad-mix)
       (translate 0 0)))

(add (with-transform
       (apply-shader! (rect 0 0 256 256 :fill "white") horiz-mix)
       (translate 256 0)))

(add (with-transform
       (apply-shader! (rect 0 0 256 256 :fill "white") vert-mix)
       (translate 512 0)))

(render "03_noise_blend.png")
