; main.pml - 2.5D 等距草地瓦片 (shader 噪声纹理)

(set-backend! "skia")
(canvas 64 64 :bg "#1E1E2E")

(define tw 40)
(define th 20)
(define td 8)
(define cx 32)
(define cy 29)

; ═══════════════════════════════════════════════════════════════════════════════
; SkSL 噪声着色器
; ═══════════════════════════════════════════════════════════════════════════════

(define grass-shader
  (string-append
    "half4 main(float2 p) {\n"
    "  float2 ip = floor(p);\n"
    "  float2 fp = p - ip;\n"
    "  float2 u = fp * fp * (3.0 - 2.0 * fp);\n"
    ; hash 噪声
    "  float a = sin(dot(ip, float2(127.1, 311.7)) + 0.001) * 43758.5453;\n"
    "  float b = sin(dot(ip + float2(1,0), float2(127.1, 311.7)) + 0.001) * 43758.5453;\n"
    "  float c = sin(dot(ip + float2(0,1), float2(127.1, 311.7)) + 0.001) * 43758.5453;\n"
    "  float d = sin(dot(ip + float2(1,1), float2(127.1, 311.7)) + 0.001) * 43758.5453;\n"
    ; value noise + smoothstep 插值
    "  float n1 = mix(fract(a), fract(b), u.x);\n"
    "  float n2 = mix(fract(c), fract(d), u.x);\n"
    "  float n = mix(n1, n2, u.y);\n"
    ; FBM 第二层细节
    "  float2 ip2 = floor(p * 2.5);\n"
    "  float n2a = fract(sin(dot(ip2, float2(269.5, 183.3)) + 0.001) * 43758.5453);\n"
    "  n = n * 0.65 + n2a * 0.35;\n"
    ; 草叶竖条纹
    "  float blade = fract(sin(floor(p.x * 2.0) * 12.9898) * 43758.5453);\n"
    "  blade = smoothstep(0.6, 0.8, blade) * 0.2;\n"
    ; 混合草地颜色
    "  half3 dark = half3(0.20, 0.40, 0.10);\n"
    "  half3 mid  = half3(0.30, 0.55, 0.16);\n"
    "  half3 lite = half3(0.44, 0.72, 0.24);\n"
    "  half3 col = mix(dark, mid, half(n));\n"
    "  col = mix(col, lite, half(n * n * 0.5 + blade));\n"
    "  return half4(col, 1.0);\n"
    "}\n"))

(define sh (shader grass-shader))

; ═══════════════════════════════════════════════════════════════════════════════
; 等距方块几何
; ═══════════════════════════════════════════════════════════════════════════════

(define (diamond cx cy w h stroke sw)
  (polygon (list
             (list cx (- cy (/ h 2)))
             (list (+ cx (/ w 2)) cy)
             (list cx (+ cy (/ h 2)))
             (list (- cx (/ w 2)) cy))
           :fill "#5A9E3A" :stroke stroke :stroke-width sw))

(define (left-face cx cy w h d fill stroke sw)
  (polygon (list
             (list (- cx (/ w 2)) cy)
             (list cx (+ cy (/ h 2)))
             (list cx (+ cy (/ h 2) d))
             (list (- cx (/ w 2)) (+ cy d)))
           :fill fill :stroke stroke :stroke-width sw))

(define (right-face cx cy w h d fill stroke sw)
  (polygon (list
             (list (+ cx (/ w 2)) cy)
             (list cx (+ cy (/ h 2)))
             (list cx (+ cy (/ h 2) d))
             (list (+ cx (/ w 2)) (+ cy d)))
           :fill fill :stroke stroke :stroke-width sw))

; ═══════════════════════════════════════════════════════════════════════════════
; 渲染
; ═══════════════════════════════════════════════════════════════════════════════

(add (left-face cx cy tw th td "#3D7A25" "#1A1A1A" 0.8))
(add (right-face cx cy tw th td "#2A5C18" "#1A1A1A" 0.8))
(add (apply-shader! (diamond cx cy tw th "#1A1A1A" 0.8) sh))

(render "grass_tile.png")
