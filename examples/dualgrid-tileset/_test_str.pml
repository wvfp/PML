(set-backend! "skia")
(println "testing multiline string...")
(define s (shader "
uniform float u_test;
half4 main(float2 xy) {
  return half4(u_test, xy.x/100.0, xy.y/100.0, 1.0);
}
"))
(println "shader compiled")
(canvas 100 100)
(define s2 (uniform-float s "u_test" 0.5))
(define go (apply-shader! (rect 0 0 100 100 :fill "#fff") s2))
(add go)
(println "rendering...")
(render "_test_str.png")
(println "done")
