;; examples/14-gradients/01_gradient_tile_modes.pml
;; 展示 linear-gradient / radial-gradient / sweep-gradient 的 :tile-mode 行为

(set-backend! "skia")
(canvas 720 420 :bg "#2d3436")

;; 标签辅助函数
(define labeled-rect
  (lambda (x y w h label grad)
    (group
      (rect x y w h :fill-gradient grad)
      (text (+ x 10) (+ y 24) label :fill "#ffffff" :font-size 16))))

;; ---- linear-gradient tile-modes ----
(add (labeled-rect 20  20 160 120 "clamp"
      (linear-gradient '((0.0 "#e17055") (1.0 "#36cd0dff"))
        :x1 0 :y1 0 :x2 0.25 :y2 0 :tile-mode "clamp")))

(add (labeled-rect 200 20 160 120 "repeat"
      (linear-gradient '((0.0 "#e17055") (1.0 "#74b9ff"))
        :x1 0 :y1 0 :x2 0.25 :y2 0 :tile-mode "repeat")))

(add (labeled-rect 380 20 160 120 "mirror"
      (linear-gradient '((0.0 "#e17055") (1.0 "#74b9ff"))
        :x1 0 :y1 0 :x2 0.25 :y2 0 :tile-mode "mirror")))

(add (labeled-rect 560 20 140 120 "decal"
      (linear-gradient '((0.0 "#e17055") (1.0 "#74b9ff"))
        :x1 0 :y1 0 :x2 0.75 :y2 0 :tile-mode "decal")))

;; ---- radial-gradient tile-mode "decal" ----
(add (labeled-rect 20  180 220 160 "radial decal"
      (radial-gradient '((0.0 "#28d0afff") (1.0 "#0984e3"))
        :cx 0.5 :cy 0.5 :r 0.35 :tile-mode "decal")))

;; ---- sweep-gradient tile-mode "repeat" ----
(add (labeled-rect 260 180 220 160 "sweep repeat"
      (sweep-gradient '((0.0 "#fdcb6e") (0.5 "#d63031") (1.0 "#fdcb6e"))
        :cx 0.5 :cy 0.5 :start-angle 0 :end-angle 180 :tile-mode "repeat")))

;; ---- default linear gradient (clamp, full rect) ----
(add (labeled-rect 500 180 180 160 "linear default"
      (linear-gradient '((0.0 "#e17055") (1.0 "#74b9ff"))
        :x1 0 :y1 0 :x2 1 :y2 1)))

(render "01_gradient_tile_modes.png")
