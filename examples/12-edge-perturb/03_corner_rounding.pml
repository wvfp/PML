; ═══════════════════════════════════════════════════════════════════════════════
; Edge Perturbation — Corner Rounding
; ───────────────────────────────────────────────────────────────────────────────
; Demonstrates corner-radius with various settings, corner-mask, and
; combination with edge-noise.
; ═══════════════════════════════════════════════════════════════════════════════

(set-backend! "skia")
(canvas 640 360 :bg "#F5F0EB")

(define (label x y msg)
  (add (text x y msg :fill "#666" :font-size 11)))

; ── Row 1: Corner radius comparison ──────────────────────────────────────────
(define sq '((20 50) (120 50) (120 150) (20 150)))

; Exact square (no rounding)
(add (polygon sq :fill "#95a5a6" :stroke "#7f8c8d" :stroke-width 2))
(label 30 165 "r=0 (exact)")

; Small rounding
(add (with-transform
       (polygon sq :corner-radius 3.0 :fill "#e74c3c" :stroke "#c0392b" :stroke-width 2)
       (translate 140 0)))
(label 175 165 "r=3.0")

; Medium rounding
(add (with-transform
       (polygon sq :corner-radius 8.0 :fill "#3498db" :stroke "#2980b9" :stroke-width 2)
       (translate 280 0)))
(label 315 165 "r=8.0")

; Large rounding
(add (with-transform
       (polygon sq :corner-radius 15.0 :fill "#2ecc71" :stroke "#27ae60" :stroke-width 2)
       (translate 420 0)))
(label 450 165 "r=15.0")

; ── Row 2: Corner mask — selective corner rounding ───────────────────────────
(define hex-pts '((30 240) (90 210) (150 240) (150 300) (90 330) (30 300)))

; Round only corners 0 and 3 (top-left and bottom-left)
(add (polygon hex-pts :corner-radius 6.0 :corner-mask '(#t #f #f #t #f #f)
               :edge-subdiv 1
               :fill "#e67e22" :stroke "#d35400" :stroke-width 2))
(label 20 345 "Corner-mask: #t #f #f #t #f #f")

; ── Row 3: Corner rounding + edge noise combined ─────────────────────────────
(add (with-transform
       (polygon hex-pts
                :corner-radius 5.0
                :corner-mask '(#t #t #t #t #t #t)
                :edge-noise 0.15
                :edge-subdiv 3
                :edge-seed 42
                :fill "#9b59b6" :stroke "#8e44ad" :stroke-width 2)
       (translate 250 0)))
(label 280 345 "Corner-rounding + Edge-noise")

; ── Row 4: Triangle with corner rounding ─────────────────────────────────────
(define tri '((460 240) (410 320) (520 320)))

(add (with-transform
       (polygon tri :corner-radius 6.0
                :edge-subdiv 2
                :fill "#1abc9c" :stroke "#16a085" :stroke-width 2)
       (translate 440 0)))
(label 460 345 "Triangle + rounding")

(render "03_corner_rounding.png")
