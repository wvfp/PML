; grass_top_tex.pml — 草地顶面纹理 (噪声 shader)
; 提供：grass-shader (量化噪声 shader handle)

(provide grass-shader)

; ═══════════════════════════════════════════════════════════════════════════════
; 动漫草地 shader: 无缝噪声 + 3 色阶量化
; ═══════════════════════════════════════════════════════════════════════════════

(define grass-noise
  (noise-fractal :seed 42 :octaves 3
                 :freq-x 0.03 :freq-y 0.03
                 :tile-width 300 :tile-height 150))

(define grass-shader
  (quantize-noise grass-noise
    :levels '((0.30 "#267A16")
              (0.65 "#449e2d")
              (1.0  "#74c44f"))))
