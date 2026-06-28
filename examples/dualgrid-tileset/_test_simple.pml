(set-backend! "skia")
(println "test simpler...")
(define s (shader "
uniform float u_type;
half4 main(float2 xy) {
    float n = sin(xy.x*0.1)*0.5+0.5;
    if (u_type < 0.5) { return half4(n, 0.3, 0.1, 1.0); }
    else { return half4(0.2, n, 0.1, 1.0); }
}
"))
(println "compiled")
(canvas 100 100)
(define s2 (uniform-float s "u_type" 0.0))
(define go (apply-shader! (rect 0 0 100 100 :fill "#fff") s2))
(add go)
(render "_test_simple.png")
(println "done")
