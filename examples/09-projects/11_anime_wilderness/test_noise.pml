(set-backend! "skia")
(canvas 256 256 :bg "#1E1E2E")

(define noise-shader
  (shader
    (string-append
      "half4 main(float2 p) {\n"
      "  float h = fract(sin(dot(floor(p), float2(127.1, 311.7)) + 0.001) * 43758.5453);\n"
      "  float h2 = fract(sin(dot(floor(p + float2(1,0)), float2(127.1, 311.7)) + 0.001) * 43758.5453);\n"
      "  float h3 = fract(sin(dot(floor(p + float2(0,1)), float2(127.1, 311.7)) + 0.001) * 43758.5453);\n"
      "  float h4 = fract(sin(dot(floor(p + float2(1,1)), float2(127.1, 311.7)) + 0.001) * 43758.5453);\n"
      "  float2 f = fract(p);\n"
      "  float2 u = f * f * (3.0 - 2.0 * f);\n"
      "  float n = mix(mix(h, h2, u.x), mix(h3, h4, u.x), u.y);\n"
      "  half3 green = half3(0.3, 0.6, 0.2);\n"
      "  half3 dark = half3(0.15, 0.35, 0.1);\n"
      "  half3 c = mix(dark, green, half(n));\n"
      "  return half4(c, 1.0);\n"
      "}\n")))

(define diamond (polygon (list (list 128 28) (list 228 128) (list 128 228) (list 28 128))
                         :fill "#00FF00" :stroke "#111" :stroke-width 2))

(add (apply-shader! diamond noise-shader))

(render "test_noise.png")
