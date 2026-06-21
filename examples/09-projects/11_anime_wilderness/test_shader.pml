(set-backend! "skia")
(canvas 128 128 :bg "#1E1E2E")

(define test-shader
  (shader "half4 main(float2 p) { return half4(0.8, 0.2, 0.2, 1.0); }"))

(define red-rect (rect 20 20 88 88 :fill "#00FF00" :stroke "none"))

(add (apply-shader! red-rect test-shader))

(render "test_shader.png")
