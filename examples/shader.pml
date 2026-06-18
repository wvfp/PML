;; PML Shader Demo — applying post-process effects to canvas output
;;
;; Run:  uv run pml examples/shader.pml -o examples_output
;;       uv run pml examples/shader.pml -o examples_output --watch
;;
;; Each section renders the same scene with a different shader effect,
;; plus a final composite with chained effects.

;; ======================================================================
;; 1. Original (no shader)
;; ======================================================================
(canvas 200 200 :bg "#2c3e50")
(add (rect 20 20 80 80 :fill "#e74c3c" :stroke "#c0392b" :stroke-width 2))
(add (circle 140 60 30 :fill "#3498db"))
(add (text 20 120 "Original" :fill "white" :size 14))
(render "shader-original.png")

;; ======================================================================
;; 2. Sepia — warm vintage tone
;; ======================================================================
(canvas 200 200 :bg "#2c3e50")
(add (rect 20 20 80 80 :fill "#e74c3c" :stroke "#c0392b" :stroke-width 2))
(add (circle 140 60 30 :fill "#3498db"))
(add (text 20 120 "Sepia" :fill "white" :size 14))
(post-process 'sepia)
(render "shader-sepia.png")

;; ======================================================================
;; 3. Grayscale — desaturated
;; ======================================================================
(canvas 200 200 :bg "#2c3e50")
(add (rect 20 20 80 80 :fill "#e74c3c" :stroke "#c0392b" :stroke-width 2))
(add (circle 140 60 30 :fill "#3498db"))
(add (text 20 120 "Grayscale" :fill "white" :size 14))
(post-process 'grayscale)
(render "shader-grayscale.png")

;; ======================================================================
;; 4. Invert — negative colors
;; ======================================================================
(canvas 200 200 :bg "#2c3e50")
(add (rect 20 20 80 80 :fill "#e74c3c" :stroke "#c0392b" :stroke-width 2))
(add (circle 140 60 30 :fill "#3498db"))
(add (text 20 120 "Invert" :fill "white" :size 14))
(post-process 'invert)
(render "shader-invert.png")

;; ======================================================================
;; 5. Bloom — glowing highlights
;; ======================================================================
(canvas 200 200 :bg "#1a1a2e")
(add (rect 20 20 80 80 :fill "#e74c3c" :stroke "#ff6b6b" :stroke-width 2))
(add (circle 140 60 30 :fill "#ffd93d"))
(add (text 10 130 "Bloom (glow)" :fill "white" :size 14))
(post-process 'bloom :radius 6 :strength 0.4)
(render "shader-bloom.png")

;; ======================================================================
;; 6. Oil Paint — cartoon effect
;; ======================================================================
(canvas 200 200 :bg "#2c3e50")
(add (rect 20 20 80 80 :fill "#e74c3c" :stroke "#c0392b" :stroke-width 2))
(add (circle 140 60 30 :fill "#3498db"))
(add (text 10 130 "Oil Paint" :fill "white" :size 14))
(post-process 'oil-paint :range 3 :levels 6)
(render "shader-oilpaint.png")

;; ======================================================================
;; 7. Pixelate — blocky
;; ======================================================================
(canvas 200 200 :bg "#2c3e50")
(add (rect 20 20 80 80 :fill "#e74c3c" :stroke "#c0392b" :stroke-width 2))
(add (circle 140 60 30 :fill "#3498db"))
(add (text 20 120 "Pixelate" :fill "white" :size 14))
(post-process 'pixelate :size 6)
(render "shader-pixelate.png")

;; ======================================================================
;; 8. Vignette — darkened corners
;; ======================================================================
(canvas 200 200 :bg "#2c3e50")
(add (rect 20 20 80 80 :fill "#e74c3c" :stroke "#c0392b" :stroke-width 2))
(add (circle 140 60 30 :fill "#3498db"))
(add (text 20 120 "Vignette" :fill "white" :size 14))
(post-process 'vignette :strength 0.6)
(render "shader-vignette.png")

;; ======================================================================
;; 9. CRT Scanline — retro monitor
;; ======================================================================
(canvas 200 200 :bg "#2c3e50")
(add (rect 20 20 80 80 :fill "#e74c3c" :stroke "#c0392b" :stroke-width 2))
(add (circle 140 60 30 :fill "#3498db"))
(add (text 10 130 "CRT Scanline" :fill "white" :size 14))
(post-process 'crt-scanline :strength 0.25)
(render "shader-crt-scanline.png")

;; ======================================================================
;; 10. Edge Detect — outlines only
;; ======================================================================
(canvas 200 200 :bg "white")
(add (rect 20 20 80 80 :fill "#e74c3c" :stroke "#c0392b" :stroke-width 2))
(add (circle 140 60 30 :fill "#3498db"))
(add (text 20 120 "Edge Detect" :fill "black" :size 14))
(post-process 'edge-detect)
(render "shader-edge-detect.png")

;; ======================================================================
;; 11. Noise — film grain
;; ======================================================================
(canvas 200 200 :bg "#2c3e50")
(add (rect 20 20 80 80 :fill "#e74c3c" :stroke "#c0392b" :stroke-width 2))
(add (circle 140 60 30 :fill "#3498db"))
(add (text 20 120 "Film Grain" :fill "white" :size 14))
(post-process 'noise :amount 0.15)
(render "shader-noise.png")

;; ======================================================================
;; 12. Chained effects: grayscale + vignette + noise
;; ======================================================================
(canvas 200 200 :bg "#2c3e50")
(add (rect 20 20 80 80 :fill "#e74c3c" :stroke "#c0392b" :stroke-width 2))
(add (circle 140 60 30 :fill "#3498db"))
(add (text 10 130 "Grayscale + Vignette + Noise" :fill "white" :size 12))
(post-process 'grayscale)
(post-process 'vignette :strength 0.5)
(post-process 'noise :amount 0.08 :monochrome #t)
(render "shader-chained.png")

(print "Done! Rendered 12 shader variants to examples_output/")
(list-shaders)
