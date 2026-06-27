;; 示例 7.5：自定义 SkSL 着色器
;; 展示 (shader ...) 编译内联 SkSL 源代码
;; 输出: 05_sksl_shader.png

(set-backend! "skia")
(canvas 400 400 :bg "#1a1a2e")

;; ---- 基本着色器：渐变 ----------------------------------------------------------------─
(define gradient-shader (shader "
  half4 main(float2 xy) {
    float r = xy.x / 400.0;
    float g = xy.y / 400.0;
    float b = 1.0 - (xy.x + xy.y) / 800.0;
    return half4(r, g, b, 1.0);
  }
"))

(add (apply-shader! (rect 0 0 200 200) gradient-shader))

;; ---- 条纹着色器：三角函数图案 ----------------------------------------------------------------─
(define stripe-shader (shader "
  half4 main(float2 xy) {
    float stripe = sin(xy.x * 0.1) * cos(xy.y * 0.1);
    stripe = stripe * 0.5 + 0.5;
    return half4(stripe, stripe * 0.3, stripe * 0.8, 1.0);
  }
"))

(add (with-transform
       (apply-shader! (rect 200 0 200 200) stripe-shader)
       (translate 0 0)))

;; ---- 径向光晕 ----------------------------------------------------------------─
(define glow-shader (shader "
  half4 main(float2 xy) {
    float2 center = float2(100.0, 100.0);
    float dist = length(xy - center) / 100.0;
    float glow = exp(-dist * dist * 3.0);
    return half4(glow * 1.0, glow * 0.3, glow * 0.1, 1.0);
  }
"))

(add (with-transform
       (apply-shader! (rect 0 200 200 200) glow-shader)
       (translate 0 0)))

;; ---- 棋盘格 ----------------------------------------------------------------─
(define checker-shader (shader "
  half4 main(float2 xy) {
    float2 grid = floor(xy / 25.0);
    float c = mod(grid.x + grid.y, 2.0);
    return half4(c * 0.9, c * 0.9, c * 0.9, 1.0);
  }
"))

(add (with-transform
       (apply-shader! (rect 200 200 200 200) checker-shader)
       (translate 0 0)))

(render "05_sksl_shader.png")
