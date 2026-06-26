;; 示例 7.1：Voronoi（细胞）噪声
;; 展示 noise-voronoi 不同 cell-size / jitter 的效果
;; 输出: 01_voronoi_noise.png

(set-backend! "skia")
(canvas 512 512 :bg "#1a1a2e")

;; ---- Voronoi 对比排列（2×2 网格） ------------------------------------------------─

;; 左上：小细胞、低 jitter
(define v-top-left (noise-voronoi :cell-size 16 :seed 0 :jitter 0.2))
(add (with-transform
       (apply-shader! (rect 0 0 256 256 :fill "white") v-top-left)
       (translate 0 0)))

;; 右上：中等细胞、中等 jitter
(define v-top-right (noise-voronoi :cell-size 32 :seed 42 :jitter 0.5))
(add (with-transform
       (apply-shader! (rect 0 0 256 256 :fill "white") v-top-right)
       (translate 256 0)))

;; 左下：大细胞、高 jitter
(define v-bottom-left (noise-voronoi :cell-size 64 :seed 7 :jitter 0.9))
(add (with-transform
       (apply-shader! (rect 0 0 256 256 :fill "white") v-bottom-left)
       (translate 0 256)))

;; 右下：中细胞、无缝平铺（可做 tile 纹理）
(define v-bottom-right (noise-voronoi :cell-size 32 :seed 123 :jitter 0.4))
(add (with-transform
       (apply-shader! (rect 0 0 256 256 :fill "white") v-bottom-right)
       (translate 256 256)))

(render "01_voronoi_noise.png")
