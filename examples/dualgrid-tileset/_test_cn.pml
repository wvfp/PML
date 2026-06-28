(set-backend! "skia")
(println "testing chinese in shader...")
(define s (shader "
uniform float u_test;
// 测试 — 中文注释
half4 main(float2 xy) {
  return half4(u_test, xy.x/100.0, xy.y/100.0, 1.0);
}
"))
(println "shader compiled OK")
(canvas 100 100)
(define s2 (uniform-float s "u_test" 0.5))
(define go (apply-shader! (rect 0 0 100 100 :fill "#fff") s2))
(add go)
(render "_test_cn.png")
(println "done")
