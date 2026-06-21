;; ══════════════════════════════════════════════════════════════════
;; PML Edge Case Tests — F3 Real Manual QA
;; ══════════════════════════════════════════════════════════════════

;; EC1: render-tilemap with nonexistent tilemap → error
(println "EC1: render-tilemap 'nonexistent")
(let ((result (with-exception-handler
                (lambda (err) (list 'caught err))
                (lambda () (render-tilemap 'nonexistent :output "x.png")))))
  (println (if (and (pair? result) (eq? (car result) 'caught))
              "  PASS: correctly errored"
              (string-append "  FAIL: expected error, got " (format "~a" result)))))

;; EC2: tilemap-set! with nonexistent tilemap → error
(println "\nEC2: tilemap-set! 'nonexistent")
(let ((result (with-exception-handler
                (lambda (err) (list 'caught err))
                (lambda () (tilemap-set! 'nonexistent 0 0 1)))))
  (println (if (and (pair? result) (eq? (car result) 'caught))
              "  PASS: correctly errored"
              (string-append "  FAIL: expected error, got " (format "~a" result)))))

;; EC3: tilemap-set! with tile ID 0 (empty) → should set then render empty
(println "\nEC3: tilemap-set! with tile 0 (empty)")
(define-tileset 'empty-ts :tile-size 16 :tiles '((1 tile (rect 0 0 16 16 :fill "red"))))
(define ec3-tm (make-tilemap 'empty-ts 3 3 :projection 'orthogonal :layers 1))
(tilemap-set! ec3-tm 0 0 0 0)  ;; tile ID 0 = empty
(println "  tilemap-set! with tile 0: OK (no crash)")
(render-tilemap ec3-tm :output ".omo/evidence/final-qa/edge_empty_tilemap.png")
(println "  render-tilemap (empty): OK")

;; EC4: render-channels with invalid sprite (integer) → error
(println "\nEC4: render-channels with integer sprite")
(let ((result (with-exception-handler
                (lambda (err) (list 'caught err))
                (lambda () (render-channels 42 :channels '(albedo))))))
  (println (if (and (pair? result) (eq? (car result) 'caught))
              "  PASS: correctly errored"
              (string-append "  FAIL: expected error, got " (format "~a" result)))))

;; EC5: bind-textures with nonexistent handle → error
(println "\nEC5: bind-textures 'nonexistent")
(let ((result (with-exception-handler
                (lambda (err) (list 'caught err))
                (lambda () (bind-textures 'nonexistent :textures '())))))
  (println (if (and (pair? result) (eq? (car result) 'caught))
              "  PASS: correctly errored"
              (string-append "  FAIL: expected error, got " (format "~a" result)))))

;; EC6: bind-textures with handle 0 → error
(println "\nEC6: bind-textures 0")
(let ((result (with-exception-handler
                (lambda (err) (list 'caught err))
                (lambda () (bind-textures 0 :textures '())))))
  (println (if (and (pair? result) (eq? (car result) 'caught))
              "  PASS: correctly errored"
              (string-append "  FAIL: expected error, got " (format "~a" result)))))

(println "\n═══ All edge case tests completed ═══")
