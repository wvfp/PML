(set-backend! "skia")

;; Test 1: Simple rect as texture
(define test-go (rect 0 0 64 64 :fill "#e94560"))
(define s (shader "uniform shader tex_a; half4 main(float2 xy) { return tex_a.eval(xy); }"))
(define bound (bind-textures s :textures (list (list "tex_a" test-go))))
(println "Test 1 (simple rect): bind-textures OK")

(canvas 64 64 :bg "#fff")
(define go (apply-shader! (rect 0 0 64 64) bound))
(add go)
(render "_test_bind.png")
(println "  rendered OK")
