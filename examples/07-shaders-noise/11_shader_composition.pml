;; ========================================
;; GPU Shader 组合：compose-with-child, compose-with-children
;; ========================================
;; 运行: pml.exe 11_shader_composition.pml

(set-backend! "skia")
(canvas 400 300 :bg "#1a1a2e")

;; ---- 4a: compose-with-child — 用 SkSL 包裹 child shader ----
(print "=== compose-with-child ===")

;; 基础噪声
(define perlin (noise-fractal :seed 42 :octaves 6 :freq-x 0.01 :freq-y 0.01))

;; 用量化 SkSL 包裹噪声 → 地形风格色块
(define terrain (compose-with-child "
  uniform shader src;
  half4 main(float2 xy) {
    half4 n = src.eval(xy);
    float v = (n.r + n.g + n.b) * 0.333;
    if (v < 0.33) return half4(0.1, 0.1, 0.3, 1.0);  // deep
    if (v < 0.50) return half4(0.2, 0.5, 0.2, 1.0);  // forest
    if (v < 0.66) return half4(0.5, 0.7, 0.2, 1.0);  // grass
    return half4(0.7, 0.8, 0.4, 1.0);                  // highland
  }
" perlin))

(add (apply-shader! (rect 0 0 200 300) terrain :coordinate-space :world))

;; ---- compose-with-children: 混合两个噪声 ----
(print "=== compose-with-children ===")
(define noise-a (noise-fractal :seed 1 :octaves 4 :freq-x 0.015 :freq-y 0.015))
(define noise-b (noise-turbulence :seed 2 :octaves 4 :freq-x 0.02 :freq-y 0.02))

(define blended (compose-with-children "
  uniform shader child_0;
  uniform shader child_1;
  uniform float u_mix;
  half4 main(float2 xy) {
    return mix(child_0.eval(xy), child_1.eval(xy), u_mix);
  }
" (list noise-a noise-b) :uniforms (make-uniforms 0.6)))

(add (apply-shader! (rect 200 0 200 300) blended :coordinate-space :world))

(render "11_shader_composition.png")
