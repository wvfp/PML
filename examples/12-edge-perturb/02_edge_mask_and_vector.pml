; ═══════════════════════════════════════════════════════════════════════════════
; Edge Perturbation — Mask & Vector Per-Edge Control
; ───────────────────────────────────────────────────────────────────────────────
; Shows how to control which edges get perturbed (edge-mask) and use
; per-edge vector parameters for edge-noise and edge-subdiv.
; ═══════════════════════════════════════════════════════════════════════════════

(set-backend! "skia")
(canvas 640 360 :bg "#F5F0EB")

(define (label x y msg)
  (add (text x y msg :fill "#666" :font-size 11)))

; ── Row 1: Only perturb sides 0 and 2 (top & bottom) ─────────────────────────
; Square with edge-mask: keep vertical edges straight
(define sq '((20 50) (120 50) (120 150) (20 150)))
(define masked1 (perturb-polygon sq
                   :edge-noise '(0.2 0.0 0.2 0.0)
                   :edge-subdiv '(4 0 4 0)
                   :edge-mask '(#t #f #t #f)
                   :seed 31))
(add (polygon masked1 :fill "#e67e22" :stroke "#d35400" :stroke-width 2))
(label 20 165 "Mask: only top/bottom")

; ── Row 2: Only perturb sides 1 and 3 (left & right) ─────────────────────────
(define masked2 (perturb-polygon sq
                   :edge-noise '(0.0 0.2 0.0 0.25)
                   :edge-subdiv '(0 4 0 4)
                   :edge-mask '(#f #t #f #t)
                   :seed 31))
(add (with-transform
       (polygon masked2 :fill "#8e44ad" :stroke "#6c3483" :stroke-width 2)
       (translate 150 0)))
(label 170 165 "Mask: only left/right")

; ── Row 3: Per-edge noise amplitude ──────────────────────────────────────────
; Each side gets a different intensity
(define per-edge (perturb-polygon sq
                    :edge-noise '(0.05 0.15 0.30 0.10)
                    :edge-subdiv '(2 4 6 3)
                    :edge-mask '(#t #t #t #t)
                    :seed 42))
(add (with-transform
       (polygon per-edge :fill "#1abc9c" :stroke "#16a085" :stroke-width 2)
       (translate 300 0)))
(label 320 165 "Per-edge noise & subdiv")

; ── Row 4: Single edge masked (only bottom) ──────────────────────────────────
(define single-edge (perturb-polygon sq
                      :edge-noise 0.3
                      :edge-subdiv 5
                      :edge-mask '(#f #f #t #f)
                      :seed 99))
(add (with-transform
       (polygon single-edge :fill "#f39c12" :stroke "#e67e22" :stroke-width 2)
       (translate 450 0)))
(label 470 165 "Only bottom edge")

; ── Footer explanation ───────────────────────────────────────────────────────
(label 20 350
  "edge-mask: #t = perturbed, #f = straight")

(render "02_edge_mask_and_vector.png")
