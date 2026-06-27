;; 示例 7.9：无缝平铺噪声
;; 展示 noise-fractal / noise-turbulence 的 :tile-w / :tile-h 参数
;; 输出: 09_seamless_noise.png

(set-backend! "skia")
(canvas 512 512 :bg "#1a1a2e")

;; ---- 对比：普通噪声 vs 无缝平铺噪声 ----------------------------------------------------------------─

;; 左上：普通分形噪声（有接缝）
(define raw (noise-fractal :seed 42 :octaves 5 :freq-x 0.02 :freq-y 0.02))
(add (with-transform
       (apply-shader! (rect 0 0 256 256) raw)
       (translate 0 0)))
(add (text 128 250 "Raw (seamed)" :fill "#aaa" :font-size 11 :align 'center))

;; 右上：无缝平铺分形噪声 256×256
(define seamless (noise-fractal :seed 42 :octaves 5 :freq-x 0.02 :freq-y 0.02
                  :tile-w 256 :tile-h 256))
(add (with-transform
       (apply-shader! (rect 0 0 256 256) seamless)
       (translate 256 0)))
(add (text 384 250 "Seamless 256×256" :fill "#aaa" :font-size 11 :align 'center))

;; 左下：普通湍流噪声（有接缝）
(define raw-turb (noise-turbulence :seed 99 :octaves 4 :freq-x 0.025 :freq-y 0.025))
(add (with-transform
       (apply-shader! (rect 0 0 256 256) raw-turb)
       (translate 0 256)))
(add (text 128 510 "Raw turbulence" :fill "#aaa" :font-size 11 :align 'center))

;; 右下：无缝平铺湍流噪声
(define seamless-turb (noise-turbulence :seed 99 :octaves 4 :freq-x 0.025 :freq-y 0.025
                       :tile-w 256 :tile-h 256))
(add (with-transform
       (apply-shader! (rect 0 0 256 256) seamless-turb)
       (translate 256 256)))
(add (text 384 510 "Seamless turbulence" :fill "#aaa" :font-size 11 :align 'center))

(render "09_seamless_noise.png")

;; ---- 附加应用：无缝纹理制作 ----------------------------------------------------------------------─

;; 生成一张无缝砖墙纹理
(print "生成无缝砖墙纹理...")

(canvas 256 256 :bg "#ffffff")

(define brick-noise (noise-turbulence :seed 7 :octaves 3 :freq-x 0.04 :freq-y 0.04
                     :tile-w 256 :tile-h 256))

(define brick-quantized
  (quantize-noise brick-noise
    :levels '((0.4 "#8b4513")     ;; 砖色
              (0.7 "#a0522d")     ;; 浅砖
              (1.0 "#6b3410"))))  ;; 深砖

(add (apply-shader! (rect 0 0 256 256) brick-quantized))

(render "09_seamless_brick.png")

(print "✓ 所有无缝噪声示例已渲染完成")
