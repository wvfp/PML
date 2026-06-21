;; 测试噪声着色器功能
;; 这个示例展示如何创建无缝噪声纹理

(set-backend! "skia")
(canvas 256 256 :bg "#1E1E2E")

;; 创建分形噪声着色器 (无缝)
(define noise-shader
  (noise-fractal
    :freq-x 0.05
    :freq-y 0.05
    :octaves 4
    :seed 42
    :tile-width 256
    :tile-height 256))

;; 创建湍流噪声着色器 (无缝)
(define turbulence-shader
  (noise-turbulence
    :freq-x 0.03
    :freq-y 0.03
    :octaves 3
    :seed 123
    :tile-width 256
    :tile-height 256))

;; 创建一个矩形并应用噪声着色器
(define rect1 (rect 0 0 200 200 :fill "#FFF" :stroke "#000" :stroke-width 2))
(define rect2 (rect 28 28 200 200 :fill "#FFF" :stroke "#000" :stroke-width 2))

;; 应用分形噪声 (绿色草原)
(define grass-shader
  (shader
    (string-append
      "half4 main(float2 p) {\n"
      "  half n = half(sin(p.x * 0.05) * 0.5 + 0.5);\n"
      "  half3 dark = half3(0.1, 0.3, 0.1);\n"
      "  half3 light = half3(0.3, 0.7, 0.2);\n"
      "  return half4(mix(dark, light, half(n)), 1.0);\n"
      "}\n")))

;; 应用着色器到图形对象
(add (apply-shader! rect1 noise-shader))
(add (apply-shader! rect2 turbulence-shader))

;; 渲染输出
(render "test_noise_shader.png")
