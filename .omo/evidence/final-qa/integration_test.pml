;; ══════════════════════════════════════════════════════════════════
;; PML Integration Test — Tilemap + Render Channels
;; Part of F3 Real Manual QA
;; ══════════════════════════════════════════════════════════════════

(println "\n═══ Integration Test: define-tileset → make-tilemap → render-tilemap (ortho) ═══")
(define-tileset 'terrain :tile-size 32 :tiles '((1 grass (rect 0 0 32 32 :fill "green")) (2 stone (rect 0 0 32 32 :fill "#888"))))
(println "  define-tileset: OK")

(define tm (make-tilemap 'terrain 5 5 :projection 'orthogonal :layers 2))
(println "  make-tilemap (ortho 5x5): OK")

(tilemap-set! tm 0 2 2 1)
(println "  tilemap-set! (layer0,col2,row2,tile1 grass): OK")

(tilemap-set! tm 1 0 0 2)
(println "  tilemap-set! (layer1,col0,row0,tile2 stone): OK")

(render-tilemap tm :output ".omo/evidence/final-qa/ortho_tilemap.png")
(println "  render-tilemap (ortho): OK")

(println "\n═══ Integration Test: isometric tilemap ═══")
(define tm2 (make-tilemap 'terrain 3 3 :projection 'isometric :layers 1))
(println "  make-tilemap (iso 3x3): OK")

(tilemap-set! tm2 0 1 1 1)
(println "  tilemap-set! (layer0,col1,row1,tile1 grass): OK")

(render-tilemap tm2 :projection 'isometric :output ".omo/evidence/final-qa/iso_tilemap.png")
(println "  render-tilemap (iso): OK")

(println "\n═══ Integration Test: render-channels ═══")
(define sprite (rect 0 0 32 32 :fill "red" :stroke "black"))
(println "  rect sprite: OK")

(render-channels sprite :output ".omo/evidence/final-qa/sprite" :channels '(albedo specular normal))
(println "  render-channels: OK")

(println "\n═══ All integration tests passed ═══")
