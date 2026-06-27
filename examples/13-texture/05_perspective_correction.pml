;; 示例 13.5：透视纹理映射
;; 展示 texture-map 的 :perspective-correction 参数
;; 输出: 05_perspective_correction.png

(set-backend! "skia")

;; ---- 创建棋盘格纹理 ----------------------------------------------------------------─
(define-texture checker (64 64)
  (group
    (rect 0  0 32 32 :fill "#e94560")
    (rect 32 0 32 32 :fill "#16213e")
    (rect 0 32 32 32 :fill "#16213e")
    (rect 32 32 32 32 :fill "#e94560")))

;; ---- 对比：仿射映射 vs 透视校正 ------------------------------------------------------------------─

(canvas 420 250 :bg "#ffffff")

;; 文字标题
(add (text 105 15 "Affine" :fill "#333" :font-size 13 :align 'center))
(add (text 315 15 "Perspective Corrected" :fill "#333" :font-size 13 :align 'center))

;; 左：普通纹理映射（仿射插值）— 梯形上的棋盘格会扭曲
(define affine-tex
  (texture-map (polygon '((10 40) (200 40) (180 230) (30 230))
                :stroke "#666" :stroke-width 1)
    :source checker
    :mode :explicit
    :uv-vertices '((0 0) (1 0) (1 1) (0 1))))

(add affine-tex)

;; 右：透视校正纹理映射 — 棋盘格梯形呈现正确的透视效果
(define perspective-tex
  (texture-map (polygon '((220 40) (410 40) (390 230) (240 230))
                :stroke "#666" :stroke-width 1)
    :source checker
    :mode :explicit
    :uv-vertices '((0 0) (1 0) (1 1) (0 1))
    :perspective-correction :true))

(add perspective-tex)

(render "05_perspective_correction.png")

;; ---- 更多形状：透视纹理对比集 ------------------------------------------------------------------─
(print "生成透视对比集...")

(canvas 400 400 :bg "#ffffff")

;; 创建带编号的纹理
(define-texture grid-text (128 128)
  (group
    (rect 0   0 64 64 :fill "#ff6b6b" :stroke "#fff")
    (rect 64  0 64 64 :fill "#51cf66" :stroke "#fff")
    (rect 0  64 64 64 :fill "#339af0" :stroke "#fff")
    (rect 64 64 64 64 :fill "#fcc419" :stroke "#fff")
    (text 32 32 "A" :fill "#fff" :font-size 20 :align 'center)
    (text 96 32 "B" :fill "#fff" :font-size 20 :align 'center)
    (text 32 96 "C" :fill "#fff" :font-size 20 :align 'center)
    (text 96 96 "D" :fill "#fff" :font-size 20 :align 'center)))

;; 上排：仿射（无透视校正）
(add (text 100 15 "Affine" :fill "#333" :font-size 12 :align 'center))
(add (text 300 15 "Affine" :fill "#333" :font-size 12 :align 'center))

(add (texture-map (polygon '((20 40) (180 40) (160 180) (40 180)))
      :source grid-text :mode :explicit
      :uv-vertices '((0 0) (1 0) (1 1) (0 1))))

(add (texture-map (polygon '((220 60) (380 30) (360 190) (240 170)))
      :source grid-text :mode :explicit
      :uv-vertices '((0 0) (1 0) (1 1) (0 1))))

;; 下排：透视校正
(add (text 100 215 "Perspective" :fill "#333" :font-size 12 :align 'center))
(add (text 300 215 "Perspective" :fill "#333" :font-size 12 :align 'center))

(add (texture-map (polygon '((20 230) (180 230) (160 380) (40 380))
                  :stroke "#666" :stroke-width 1)
      :source grid-text :mode :explicit
      :uv-vertices '((0 0) (1 0) (1 1) (0 1))
      :perspective-correction :true))

(add (texture-map (polygon '((220 230) (380 210) (360 390) (240 370))
                  :stroke "#666" :stroke-width 1)
      :source grid-text :mode :explicit
      :uv-vertices '((0 0) (1 0) (1 1) (0 1))
      :perspective-correction :true))

(render "05_perspective_compare.png")

(print "✓ 所有透视纹理示例已渲染完成")
