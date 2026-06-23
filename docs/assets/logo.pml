; ═══════════════════════════════════════════════════════════════════════
; PML Logo — hand-drawn rough style, rendered by PML itself
; ═══════════════════════════════════════════════════════════════════════

(set-backend! "skia")
(canvas 600 240 :bg "#FFF9F0")

; ── Warm background wash (rough circles) ──────────────────────────────
(add (circle 300 120 170
             :fill "#FFE8D6" :roughness 3 :bowing 1.5
             :fill-style "cross-hatch" :fill-weight 1))
(add (circle 260 100 130
             :fill "#FFD4B8" :roughness 2 :bowing 1
             :fill-style "hatch" :fill-weight 1))
(add (circle 350 140 100
             :fill "#FFF1E0" :roughness 2 :bowing 0.5))

; ── Decorative hand-drawn frame ───────────────────────────────────────
(add (rect 30 20 540 200
           :fill "none" :stroke "#2D3436"
           :stroke-width 3 :roughness 2.5 :bowing 1.2 :rx 20))
(add (rect 35 25 530 190
           :fill "none" :stroke "#E17055"
           :stroke-width 1.5 :roughness 1.5 :bowing 0.8 :rx 18))

; ── "PML" letter blocks — hand-crafted pixel-art style ────────────────
; Each letter is built from 10×10 blocks with 2px gap
(define sz 10)
(define g  2)
(define pp (+ sz g))

; ── P ─────────────────────────────────────────────────────────────────
; Vertical bar (7 blocks)
(add (rect 80 60 sz sz :fill "#E17055" :rx 2))
(add (rect 80 72 sz sz :fill "#E17055" :rx 2))
(add (rect 80 84 sz sz :fill "#E17055" :rx 2))
(add (rect 80 96 sz sz :fill "#E17055" :rx 2))
(add (rect 80 108 sz sz :fill "#E17055" :rx 2))
(add (rect 80 120 sz sz :fill "#E17055" :rx 2))
(add (rect 80 132 sz sz :fill "#E17055" :rx 2))
; Top bar
(add (rect 92 60 sz sz :fill "#E17055" :rx 2))
(add (rect 104 60 sz sz :fill "#E17055" :rx 2))
(add (rect 116 60 sz sz :fill "#E17055" :rx 2))
; Middle bar (P bowl)
(add (rect 92 96 sz sz :fill "#E17055" :rx 2))
(add (rect 104 96 sz sz :fill "#E17055" :rx 2))
(add (rect 116 96 sz sz :fill "#E17055" :rx 2))
; Right side of bowl (closing the loop)
(add (rect 116 72 sz sz :fill "#E17055" :rx 2))
(add (rect 116 84 sz sz :fill "#E17055" :rx 2))
(add (rect 128 72 sz sz :fill "#E17055" :rx 2))
(add (rect 128 84 sz sz :fill "#E17055" :rx 2))

; ── M ─────────────────────────────────────────────────────────────────
; Left vertical (7 blocks)
(add (rect 170 60 sz sz :fill "#6C5CE7" :rx 2))
(add (rect 170 72 sz sz :fill "#6C5CE7" :rx 2))
(add (rect 170 84 sz sz :fill "#6C5CE7" :rx 2))
(add (rect 170 96 sz sz :fill "#6C5CE7" :rx 2))
(add (rect 170 108 sz sz :fill "#6C5CE7" :rx 2))
(add (rect 170 120 sz sz :fill "#6C5CE7" :rx 2))
(add (rect 170 132 sz sz :fill "#6C5CE7" :rx 2))
; Right vertical (7 blocks)
(add (rect 258 60 sz sz :fill "#6C5CE7" :rx 2))
(add (rect 258 72 sz sz :fill "#6C5CE7" :rx 2))
(add (rect 258 84 sz sz :fill "#6C5CE7" :rx 2))
(add (rect 258 96 sz sz :fill "#6C5CE7" :rx 2))
(add (rect 258 108 sz sz :fill "#6C5CE7" :rx 2))
(add (rect 258 120 sz sz :fill "#6C5CE7" :rx 2))
(add (rect 258 132 sz sz :fill "#6C5CE7" :rx 2))
; Left diagonal of V
(add (rect 182 72 sz sz :fill "#6C5CE7" :rx 2))
(add (rect 194 84 sz sz :fill "#6C5CE7" :rx 2))
(add (rect 206 96 sz sz :fill "#6C5CE7" :rx 2))
; Right diagonal of V
(add (rect 246 72 sz sz :fill "#6C5CE7" :rx 2))
(add (rect 234 84 sz sz :fill "#6C5CE7" :rx 2))
(add (rect 222 96 sz sz :fill "#6C5CE7" :rx 2))
; V center bridge (connects left and right diagonals)
(add (rect 214 96 sz sz :fill "#6C5CE7" :rx 2))

; ── L ─────────────────────────────────────────────────────────────────
; Vertical bar (7 blocks)
(add (rect 300 60 sz sz :fill "#00B894" :rx 2))
(add (rect 300 72 sz sz :fill "#00B894" :rx 2))
(add (rect 300 84 sz sz :fill "#00B894" :rx 2))
(add (rect 300 96 sz sz :fill "#00B894" :rx 2))
(add (rect 300 108 sz sz :fill "#00B894" :rx 2))
(add (rect 300 120 sz sz :fill "#00B894" :rx 2))
(add (rect 300 132 sz sz :fill "#00B894" :rx 2))
; Bottom bar
(add (rect 312 132 sz sz :fill "#00B894" :rx 2))
(add (rect 324 132 sz sz :fill "#00B894" :rx 2))
(add (rect 336 132 sz sz :fill "#00B894" :rx 2))
(add (rect 348 132 sz sz :fill "#00B894" :rx 2))

; ── Hand-drawn decorative dashes around letters ───────────────────────
; P splash
(add (circle 68 55 4 :fill "#E17055" :roughness 3 :bowing 1))
(add (circle 140 58 3 :fill "#E17055" :roughness 2))
(add (circle 135 105 2.5 :fill "#FDCB6E" :roughness 2))

; M splash
(add (circle 165 52 3.5 :fill "#6C5CE7" :roughness 2.5))
(add (circle 265 50 3 :fill "#6C5CE7" :roughness 2))
(add (circle 215 118 2.5 :fill "#A29BFE" :roughness 2))

; L splash
(add (circle 295 55 3 :fill "#00B894" :roughness 2.5))
(add (circle 355 140 2.5 :fill "#55EFC4" :roughness 2))

; ── Sketchy underline ─────────────────────────────────────────────────
(add (line 80 158 360 158 :stroke "#2D3436" :stroke-width 2.5
           :roughness 3.5 :bowing 1.5))
(add (line 82 161 358 161 :stroke "#E17055" :stroke-width 1.5
           :roughness 2.5 :bowing 1))
(add (line 84 164 356 164 :stroke "#6C5CE7" :stroke-width 1
           :roughness 2 :bowing 0.5))

; ── Color splashes ────────────────────────────────────────────────────
(add (circle 480 60 12 :fill "#FDCB6E" :roughness 3 :bowing 1.5
           :fill-style "dots" :fill-weight 2))
(add (circle 500 170 10 :fill "#E17055" :roughness 2 :bowing 1
           :fill-style "cross-hatch"))
(add (circle 520 120 8 :fill "#6C5CE7" :roughness 2.5 :bowing 1.5
           :fill-style "hatch"))
(add (circle 60 175 7 :fill "#00B894" :roughness 2 :bowing 0.5))

; ── Subtle hand-drawn dots / speckles ─────────────────────────────────
(add (circle 460 50 2 :fill "#E17055" :roughness 1))
(add (circle 530 155 1.5 :fill "#2D3436" :roughness 1))
(add (circle 475 180 1.5 :fill "#6C5CE7" :roughness 1))
(add (circle 55 170 1.5 :fill "#FDCB6E" :roughness 1))

; ── Tagline ───────────────────────────────────────────────────────────
(add (text 85 185 "Picture Markup Language"
           :fill "#636E72" :font-size 15 :font-family "Georgia"))
(add (text 85 204 "Code → Image  ·  C++23  ·  Skia GPU"
           :fill "#B2BEC3" :font-size 11 :font-family "Arial"))

(render "logo.png")
