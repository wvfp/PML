;; 示例 7.6：世界坐标着色器
;; 展示 apply-shader! 的 :coordinate-space :world 参数
;; 世界坐标：着色效果固定在画布空间，不随对象平移/缩放偏移
;; 输出: 06_world_coord_shader.png

(set-backend! "skia")
(canvas 400 400 :bg "#ffffff")

;; 创建一个条纹着色器
(define stripe-shader (shader "
  half4 main(float2 xy) {
    float s = sin(xy.x * 0.08) * 0.5 + 0.5;
    return half4(s * 0.2, s * 0.6, 1.0 - s * 0.5, 1.0);
  }
"))

;; ---- 局部坐标（默认）：条纹随对象移动 ----------------------------------------------------------------─
;; 四个矩形分别平移到不同位置，条纹跟着走
(add (with-transform
       (apply-shader! (rect 0 0 80 80) stripe-shader)
       (translate 20 20)))

(add (with-transform
       (apply-shader! (rect 0 0 80 80) stripe-shader)
       (translate 120 20)))

;; ---- 世界坐标：条纹固定不动 ----------------------------------------------------------------------─
;; 四个矩形虽然在不同位置，但条纹接缝连续，像透过窗户看同一个背景
(add (with-transform
       (apply-shader! (rect 0 0 80 80) stripe-shader
             :coordinate-space :world)
       (translate 220 20)))

(add (with-transform
       (apply-shader! (rect 0 0 80 80) stripe-shader
             :coordinate-space :world)
       (translate 320 20)))

;; ---- 实用场景：背景遮罩使用世界坐标 ----------------------------------------------------------------─
;; 用世界坐标着色器做背景，确保在不同画布位置都对齐

;; 标尺文字说明
(add (text 55 130 "Local" :fill "#333" :font-size 12 :align 'center))
(add (text 270 130 "World" :fill "#333" :font-size 12 :align 'center))

;; 噪声背景（世界坐标）— 固定在整个画布上
(define bg-noise (noise-voronoi :cell-size 30 :seed 42 :jitter 0.4))
(add (apply-shader! (rect 10 150 380 240 :rx 8 :stroke "#ccc") bg-noise
      :coordinate-space :world))

;; 在噪声背景上叠加移动的元素 — 即使矩形位置变化，背景噪声始终对齐
(add (with-transform
       (circle 100 250 30 :fill "#e94560" :opacity 0.8)
       (translate 0 0)))

(add (with-transform
       (circle 200 280 30 :fill "#e94560" :opacity 0.8)
       (translate 80 0)))

(add (with-transform
       (circle 300 260 30 :fill "#e94560" :opacity 0.8)
       (translate 30 10)))

(render "06_world_coord_shader.png")
