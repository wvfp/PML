; PML Logo — rendered by PML itself

(set-backend! "skia")
(canvas 600 200 :bg "#1a1a2e")

; Background decorative circles
(add (circle 300 100 90 :fill "none" :stroke "#16213e" :stroke-width 30))
(add (circle 300 100 110 :fill "none" :stroke "#0f3460" :stroke-width 15))

; PML letters built from small blocks
(define sz 10)
(define gap 2)
(define p (+ sz gap))

; P = vertical column 7 + top bar 4 + middle bar 4
(add (rect 60 60 sz sz :fill "#e94560" :rx 2))
(add (rect 60 72 sz sz :fill "#e94560" :rx 2))
(add (rect 60 84 sz sz :fill "#e94560" :rx 2))
(add (rect 60 96 sz sz :fill "#e94560" :rx 2))
(add (rect 60 108 sz sz :fill "#e94560" :rx 2))
(add (rect 60 120 sz sz :fill "#e94560" :rx 2))
(add (rect 60 132 sz sz :fill "#e94560" :rx 2))
(add (rect 72 60 sz sz :fill "#e94560" :rx 2))
(add (rect 84 60 sz sz :fill "#e94560" :rx 2))
(add (rect 96 60 sz sz :fill "#e94560" :rx 2))
(add (rect 72 84 sz sz :fill "#e94560" :rx 2))
(add (rect 84 84 sz sz :fill "#e94560" :rx 2))
(add (rect 96 84 sz sz :fill "#e94560" :rx 2))

; M = two vertical columns + V middle
(add (rect 130 60 sz sz :fill "#533483" :rx 2))
(add (rect 130 72 sz sz :fill "#533483" :rx 2))
(add (rect 130 84 sz sz :fill "#533483" :rx 2))
(add (rect 130 96 sz sz :fill "#533483" :rx 2))
(add (rect 130 108 sz sz :fill "#533483" :rx 2))
(add (rect 130 120 sz sz :fill "#533483" :rx 2))
(add (rect 130 132 sz sz :fill "#533483" :rx 2))
(add (rect 190 60 sz sz :fill "#533483" :rx 2))
(add (rect 190 72 sz sz :fill "#533483" :rx 2))
(add (rect 190 84 sz sz :fill "#533483" :rx 2))
(add (rect 190 96 sz sz :fill "#533483" :rx 2))
(add (rect 190 108 sz sz :fill "#533483" :rx 2))
(add (rect 190 120 sz sz :fill "#533483" :rx 2))
(add (rect 190 132 sz sz :fill "#533483" :rx 2))
; V-shaped middle
(add (rect 142 72 sz sz :fill "#533483" :rx 2))
(add (rect 154 84 sz sz :fill "#533483" :rx 2))
(add (rect 166 96 sz sz :fill "#533483" :rx 2))
(add (rect 178 84 sz sz :fill "#533483" :rx 2))
(add (rect 166 72 sz sz :fill "#533483" :rx 2))

; L = vertical column + bottom bar
(add (rect 250 60 sz sz :fill "#e94560" :rx 2))
(add (rect 250 72 sz sz :fill "#e94560" :rx 2))
(add (rect 250 84 sz sz :fill "#e94560" :rx 2))
(add (rect 250 96 sz sz :fill "#e94560" :rx 2))
(add (rect 250 108 sz sz :fill "#e94560" :rx 2))
(add (rect 250 120 sz sz :fill "#e94560" :rx 2))
(add (rect 250 132 sz sz :fill "#e94560" :rx 2))
(add (rect 262 132 sz sz :fill "#e94560" :rx 2))
(add (rect 274 132 sz sz :fill "#e94560" :rx 2))
(add (rect 286 132 sz sz :fill "#e94560" :rx 2))
(add (rect 298 132 sz sz :fill "#e94560" :rx 2))

; Decorative underline
(add (rect 60 155 sz sz :fill "#e94560" :rx 2))
(add (rect 72 155 sz sz :fill "#e94560" :rx 2))
(add (rect 84 155 sz sz :fill "#533483" :rx 2))
(add (rect 96 155 sz sz :fill "#533483" :rx 2))
(add (rect 108 155 sz sz :fill "#e94560" :rx 2))
(add (rect 120 155 sz sz :fill "#e94560" :rx 2))
(add (rect 132 155 sz sz :fill "#533483" :rx 2))
(add (rect 144 155 sz sz :fill "#533483" :rx 2))
(add (rect 156 155 sz sz :fill "#e94560" :rx 2))
(add (rect 168 155 sz sz :fill "#e94560" :rx 2))
(add (rect 180 155 sz sz :fill "#533483" :rx 2))
(add (rect 192 155 sz sz :fill "#533483" :rx 2))
(add (rect 204 155 sz sz :fill "#e94560" :rx 2))
(add (rect 216 155 sz sz :fill "#e94560" :rx 2))
(add (rect 228 155 sz sz :fill "#533483" :rx 2))
(add (rect 240 155 sz sz :fill "#533483" :rx 2))
(add (rect 252 155 sz sz :fill "#e94560" :rx 2))
(add (rect 264 155 sz sz :fill "#e94560" :rx 2))
(add (rect 276 155 sz sz :fill "#533483" :rx 2))
(add (rect 288 155 sz sz :fill "#533483" :rx 2))
(add (rect 300 155 sz sz :fill "#e94560" :rx 2))
(add (rect 312 155 sz sz :fill "#e94560" :rx 2))
(add (rect 324 155 sz sz :fill "#533483" :rx 2))
(add (rect 336 155 sz sz :fill "#533483" :rx 2))
(add (rect 348 155 sz sz :fill "#e94560" :rx 2))
(add (rect 360 155 sz sz :fill "#e94560" :rx 2))

; Subtitle
(add (text 310 95 "Picture Markup Language"
           :fill "#a0a0b8" :font-size 16 :font-family "Arial"))
(add (text 310 120 "Code → Image"
           :fill "#e94560" :font-size 13 :font-family "Arial"))

(render "logo.png")
