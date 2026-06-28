;; 08_gradient_tile_mode.pml — 展示 gradient 的 :tile-mode（clamp/repeat/mirror/decal）
;; 需要使用重命名后的 gradient builtins：linear-gradient / radial-gradient / sweep-gradient

(set-backend! "skia")
(canvas 640 360 :bg "#2d3436")

;; 带标签的矩形（使用 lambda 包装，避免 defn 宏对复杂对象的传递问题）
(define labeled-rect
  (lambda (x y w h label grad)
    (group
      (rect x y w h :fill-gradient grad)
      (text (+ x 8) (+ y 20) label :fill "#ffffff" :font-size 14))))

;; ---- linear-gradient 的四种 tile-mode ----
;; 渐变向量只占矩形宽度的 25%，这样 repeat/mirror 才会看得出来。

;; 1. clamp（默认）：超出范围使用端点颜色
(add (labeled-rect 20  20 140 100 "clamp"
      (linear-gradient '((0.0 "#e17055") (1.0 "#74b9ff"))
        :x1 0 :y1 0 :x2 0.25 :y2 0 :tile-mode "clamp")))

;; 2. repeat：重复渐变
(add (labeled-rect 180 20 140 100 "repeat"
      (linear-gradient '((0.0 "#e17055") (1.0 "#74b9ff"))
        :x1 0 :y1 0 :x2 0.25 :y2 0 :tile-mode "repeat")))

;; 3. mirror：镜像重复渐变
(add (labeled-rect 340 20 140 100 "mirror"
      (linear-gradient '((0.0 "#e17055") (1.0 "#74b9ff"))
        :x1 0 :y1 0 :x2 0.25 :y2 0 :tile-mode "mirror")))

;; 4. decal：超出范围透明（露出画布背景）
;; 这里 x2=0.75，让矩形中心仍在渐变范围内，边缘才透明。
(add (labeled-rect 500 20 120 100 "decal"
      (linear-gradient '((0.0 "#e17055") (1.0 "#74b9ff"))
        :x1 0 :y1 0 :x2 0.75 :y2 0 :tile-mode "decal")))

;; ---- radial-gradient :tile-mode "decal" ----
;; 半径只有矩形的一半，四角会露出背景。
(add (labeled-rect 20 160 180 140 "radial decal"
      (radial-gradient '((0.0 "#e17055") (1.0 "#74b9ff"))
        :cx 0.5 :cy 0.5 :r 0.35 :tile-mode "decal")))

;; ---- sweep-gradient :tile-mode "repeat" ----
;; 0°–180° 扫掠，repeat 会在另一半也重复一次。
(add (labeled-rect 220 160 180 140 "sweep repeat"
      (sweep-gradient '((0.0 "#e17055") (0.5 "#74b9ff") (1.0 "#e17055"))
        :cx 0.5 :cy 0.5 :start-angle 0 :end-angle 180 :tile-mode "repeat")))

;; 普通 linear-gradient（clamp 默认）作为对照
(add (labeled-rect 420 160 180 140 "linear default"
      (linear-gradient '((0.0 "#e17055") (1.0 "#74b9ff"))
        :x1 0 :y1 0 :x2 1 :y2 1)))

(render "08_gradient_tile_mode.png")
