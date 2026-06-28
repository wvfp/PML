(set-backend! "skia")
(println "test")
(define s (shader "
uniform float u_test;
half4 main(float2 xy) {
  return half4(1,0,0,1);
}
"))
(println "ok")
