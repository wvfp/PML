;; 示例 7.2：域扭曲（Domain Warp）
;; 展示 noise-warp 用不同 warp field + amount 扭曲基础噪声
;; 输出: 02_domain_warp.png

(set-backend! "skia")
(canvas 512 512 :bg "#1a1a2e")

;; ---- 准备基础噪声和扭曲场 ----------------------------------------------------------------─

;; 基础：分形噪声（细节丰富）
(define base (noise-fractal :octaves 6 :scale 0.025 :seed 1))

;; 扭曲场 1：低频平滑噪声（产生柔和扭曲）
(define warp-smooth (noise-fractal :octaves 2 :scale 0.008 :seed 99))

;; 扭曲场 2：湍流噪声（产生剧烈扭曲）
(define warp-rough (noise-turbulence :octaves 4 :scale 0.015 :seed 77))

;; ---- 对比排列（2×2 网格） ----------------------------------------------------------------─

;; 左上：原始无扭曲（对照）
(add (with-transform
       (apply-shader! (rect 0 0 256 256 :fill "white") base)
       (translate 0 0)))

;; 右上：柔和域扭曲 (amount=20, freq=0.008)
(define mild (noise-warp base warp-smooth :amount 20.0 :freq 0.008))
(add (with-transform
       (apply-shader! (rect 0 0 256 256 :fill "white") mild)
       (translate 256 0)))

;; 左下：强域扭曲 (amount=80, freq=0.005)
(define strong (noise-warp base warp-smooth :amount 80.0 :freq 0.005))
(add (with-transform
       (apply-shader! (rect 0 0 256 256 :fill "white") strong)
       (translate 0 256)))

;; 右下：湍流扭曲场（产生破碎感）
(define turbulent (noise-warp base warp-rough :amount 50.0 :freq 0.01))
(add (with-transform
       (apply-shader! (rect 0 0 256 256 :fill "white") turbulent)
       (translate 256 256)))

(render "02_domain_warp.png")
