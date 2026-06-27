;; 示例 7.8：着色器 Uniform 数据
;; 展示 make-uniforms / apply-uniforms / uniform-float
;; 注意：SkSL uniform 命名需要与后端匹配，此处用简化的单-uniform 着色器
;; 输出: 08_uniforms.png

(set-backend! "skia")
(canvas 400 300 :bg "#1a1a2e")

;; ---- 简单 SkSL 着色器：一个 uniform 控制颜色强度 ----------------------------------------------─
(define intensity-shader (shader "
  uniform float u_strength;

  half4 main(float2 xy) {
    float r = sin(xy.x * 0.05 + u_strength) * 0.5 + 0.5;
    float g = cos(xy.y * 0.05 + u_strength * 0.7) * 0.5 + 0.5;
    float b = sin((xy.x + xy.y) * 0.03 + u_strength * 1.3) * 0.5 + 0.5;
    return half4(r, g, b, 1.0);
  }
"))

;; 测试不同 uniform 值
(define s-weak (uniform-float intensity-shader "u_strength" 0.0))
(add (with-transform
       (apply-shader! (rect 10 10 180 130 :rx 6) s-weak)
       (translate 0 0)))
(add (text 100 155 "strength=0.0" :fill "#aaa" :font-size 10 :align 'center))

(define s-strong (uniform-float intensity-shader "u_strength" 3.0))
(add (with-transform
       (apply-shader! (rect 210 10 180 130 :rx 6) s-strong)
       (translate 0 0)))
(add (text 300 155 "strength=3.0" :fill "#aaa" :font-size 10 :align 'center))

;; 用 make-uniforms + apply-uniforms 批量设置
(define data-1 (make-uniforms 1.5))
(define s1 (apply-uniforms intensity-shader data-1))
(add (with-transform
       (apply-shader! (rect 10 180 90 100 :rx 4) s1)
       (translate 0 0)))
(add (text 55 290 "make-uniforms" :fill "#aaa" :font-size 9 :align 'center))

(define data-2 (make-uniforms 5.0))
(define s2 (apply-uniforms intensity-shader data-2))
(add (with-transform
       (apply-shader! (rect 110 180 90 100 :rx 4) s2)
       (translate 0 0)))
(add (text 155 290 "another value" :fill "#aaa" :font-size 9 :align 'center))

(define data-3 (make-uniforms 8.0))
(define s3 (apply-uniforms intensity-shader data-3))
(add (with-transform
       (apply-shader! (rect 210 180 90 100 :rx 4) s3)
       (translate 0 0)))
(add (text 255 290 "more" :fill "#aaa" :font-size 9 :align 'center))

(define data-4 (make-uniforms 12.0))
(define s4 (apply-uniforms intensity-shader data-4))
(add (with-transform
       (apply-shader! (rect 310 180 80 100 :rx 4) s4)
       (translate 0 0)))
(add (text 350 290 "extreme" :fill "#aaa" :font-size 9 :align 'center))

(render "08_uniforms.png")
