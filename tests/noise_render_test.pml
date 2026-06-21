;; Render test for all three new noise builtins

;; Helper: render a graphic object to a PNG file
(define (render-graphic obj filename)
  (canvas 256 256)
  (add obj)
  (render filename))

;; --- Voronoi noise ---
(define voronoi-cell32 (noise-voronoi :cell-size 32 :seed 0 :jitter 0.5))
(define voronoi-cell16 (noise-voronoi :cell-size 16 :seed 42 :jitter 0.8))
(define voronoi-cell64 (noise-voronoi :cell-size 64 :seed 7 :jitter 0.3))

(render-graphic (rect 0 0 256 256 :fill voronoi-cell32) "test_voronoi_cell32.png")
(render-graphic (rect 0 0 256 256 :fill voronoi-cell16) "test_voronoi_cell16.png")
(render-graphic (rect 0 0 256 256 :fill voronoi-cell64) "test_voronoi_cell64.png")

;; --- Domain warp ---
(define base-frac (noise-fractal :octaves 4 :scale 0.02 :seed 1))
(define warp-field (noise-fractal :octaves 2 :scale 0.008 :seed 99))

(define warped-30  (noise-warp base-frac warp-field :amount 30.0 :freq 0.01))
(define warped-80  (noise-warp base-frac warp-field :amount 80.0 :freq 0.005))

(render-graphic (rect 0 0 256 256 :fill warped-30) "test_warp_30.png")
(render-graphic (rect 0 0 256 256 :fill warped-80) "test_warp_80.png")

;; --- Noise blend ---
(define blend-grad   (noise-blend voronoi-cell32 warped-30 :mode 'gradient :weight 0.5))
(define blend-horiz  (noise-blend voronoi-cell32 warped-30 :mode 'horizontal :weight 0.3))
(define blend-vert   (noise-blend voronoi-cell32 warped-30 :mode 'vertical :weight 0.7))

(render-graphic (rect 0 0 256 256 :fill blend-grad)  "test_blend_grad.png")
(render-graphic (rect 0 0 256 256 :fill blend-horiz) "test_blend_horiz.png")
(render-graphic (rect 0 0 256 256 :fill blend-vert)  "test_blend_vert.png")

(print "ALL RENDER TESTS DONE")
