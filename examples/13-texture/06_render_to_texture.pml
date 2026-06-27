;; 示例 13.6：纹理合成（render-to-texture）
;; 展示 render-to-texture 将 GraphicObject 渲染为纹理，再通过 texture-map 使用
;; 输出: 06_render_to_texture.png

(set-backend! "skia")

;; ---- 方法 1：define-texture（常规方式，特殊形式）-------------------------------------------------------─
(define-texture star (64 64)
  (group
    (rect 0 0 64 64 :fill "#0f0c29")
    (circle 32 16 8 :fill "#ffe066")
    (circle 10 10 1.5 :fill "#fff")
    (circle 50 20 1 :fill "#fff")
    (circle 20 40 1.5 :fill "#fff")))

;; ---- 方法 2：render-to-texture（内置函数，可在运行时动态创建）---------------------------------------------─
;; 这在 define-texture 不适用时很有用：比如纹理内容在运行时计算
(define dynamic-tex
  (render-to-texture 64 64
    (group
      (rect 0 0 64 64 :fill "#1a1a2e")
      (circle 32 32 20 :fill "#e94560" :opacity 0.8)
      (circle 20 20 10 :fill "#ff6b6b")
      (circle 44 20 10 :fill "#ff6b6b")
      (ellipse 32 44 12 6 :fill "#16213e"))))

;; ---- 应用：混合使用两种纹理 ----------------------------------------------------------------─

(canvas 400 400 :bg "#ffffff")

;; 平铺星星纹理
(add (texture-map (rect 0 0 200 200) :source star :mode :planar))

;; 平铺动态纹理
(add (texture-map (rect 200 0 200 200) :source dynamic-tex :mode :planar))

;; 在纹理上用普通形状叠加
(add (text 100 190 "define-texture" :fill "#fff" :font-size 12 :align 'center))
(add (text 300 190 "render-to-texture" :fill "#fff" :font-size 12 :align 'center))

;; ---- 高级用法：纹理链式合成 ----------------------------------------------------------------─

;; 渲染一张渐变圆作为纹理
(define gradient-tex
  (render-to-texture 128 128
    (group
      (rect 0 0 128 128 :fill "#2d3436")
      (circle 64 64 50 :fill "#fd79a8")
      (circle 64 64 30 :fill "#a29bfe"))))

;; 把渐变圆作为纹理贴到矩形上
(add (texture-map (rect 50 220 300 160 :rx 12)
      :source gradient-tex :mode :harmonic :wrap-x :repeat :wrap-y :repeat))

(add (text 200 395 "Chained: render-to-texture → texture-map"
      :fill "#333" :font-size 12 :align 'center))

(render "06_render_to_texture.png")

(print "✓ 所有纹理合成示例已渲染完成")
