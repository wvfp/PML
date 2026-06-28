;; ═══════════════════════════════════════════════════════════════════════════════
;; 双网格（Dual-Grid）瓦片系统 — PML 复刻
;;
;; 基于 TapTap 文章
;; 支持 4 种地形

(set-backend! "skia")
(println "start")

(define sksl (shader "
uniform float u_bl;
half4 main(float2 xy) {
  return half4(u_bl, xy.x/128.0, xy.y/128.0, 1.0);
}
"))
(println "  compiled OK")

(canvas 128 128)
(define s1 (uniform-float sksl "u_bl" 0.5))
(define go (apply-shader! (rect 0 0 128 128) s1))
(add go)
(render "_test_enc.png")
(println "done")
