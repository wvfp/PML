; iso_grass_scene.pml — 5×5 动漫草地顶面 + 写实侧面 等距场景

(set-backend! "skia")
(canvas 1700 950)

(import "grass_lib.pml" as lib)

(define tw 300)
(define th 150)
(define td 54)

(define ox 800)
(define oy 150)

; ═══════════════════════════════════════════════════════════════════════════════
; 坐标转换
; ═══════════════════════════════════════════════════════════════════════════════

(define (iso-to-screen col row)
  (list
    (+ ox (* (/ tw 2) (- col row)))
    (+ oy (* (/ th 2) (+ col row)))))

; ═══════════════════════════════════════════════════════════════════════════════
; 5×5 场景渲染
; ═══════════════════════════════════════════════════════════════════════════════

(define (render-scene rows cols row-idx col-idx)
  (if (< row-idx rows)
      (if (< col-idx cols)
          (begin
            (define pos (iso-to-screen col-idx row-idx))
            (add (lib/draw-grass-tile (car pos) (list-ref pos 1) tw th td lib/grass-shader))
            (render-scene rows cols row-idx (+ col-idx 1)))
          (render-scene rows cols (+ row-idx 1) 0))))

(render-scene 5 5 0 0)

(render "iso_grass_scene.png")
