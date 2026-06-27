;; 示例 7.7：噪声量化（quantize-noise）
;; 展示 quantize-noise 将连续噪声量化为离散色带
;; 输出: 07_quantize_noise.png

(set-backend! "skia")

;; ---- 场景 1：地形图着色（分形噪声 → 海拔色带）-------------------------------------------------------─
(print "生成地形图...")

(define terrain-noise (noise-fractal :seed 42 :octaves 6 :freq-x 0.008 :freq-y 0.008))

(define terrain-quantized
  (quantize-noise terrain-noise
    :levels '((0.15 "#1a5276")    ;; 深海
              (0.30 "#2e86c1")    ;; 浅海
              (0.45 "#f0e5cf")    ;; 沙滩
              (0.60 "#52be80")    ;; 草地
              (0.75 "#7d6608")    ;; 丘陵
              (0.90 "#935116")    ;; 山地
              (1.00 "#fdfefe")))) ;; 雪顶

(canvas 300 300 :bg "#1a1a2e")
(add (apply-shader! (rect 0 0 300 300) terrain-quantized))
(add (text 150 290 "Terrain" :fill "#fff" :font-size 12 :align 'center))
(render "07_quantize_terrain.png")

;; ---- 场景 2：热力图（湍流噪声 → 温度色带）------------------------------------------------------------─
(print "生成热力图...")

(define heat-noise (noise-turbulence :seed 99 :octaves 4 :freq-x 0.015 :freq-y 0.015))

(define heat-quantized
  (quantize-noise heat-noise
    :levels '((0.2 "#000080")     ;; 深蓝（低温）
              (0.4 "#0000ff")     ;; 蓝
              (0.6 "#ff0000")     ;; 红
              (0.8 "#ff4500")     ;; 橙红
              (1.0 "#ffff00"))))  ;; 黄白

(canvas 300 300 :bg "#1a1a2e")
(add (apply-shader! (rect 0 0 300 300) heat-quantized))
(add (text 150 290 "Heatmap" :fill "#fff" :font-size 12 :align 'center))
(render "07_quantize_heatmap.png")

;; ---- 场景 3：对比：原始噪声 vs 量化 ----------------------------------------------------------------─
(print "生成对比图...")

(canvas 512 300 :bg "#ffffff")

;; 左：原始连续噪声
(define raw-noise (noise-fractal :seed 7 :octaves 4 :freq-x 0.01 :freq-y 0.01))
(add (with-transform
       (apply-shader! (rect 0 0 256 300) raw-noise)
       (translate 0 0)))

;; 右：量化后
(define quantized
  (quantize-noise raw-noise
    :levels '((0.25 "#e74c3c") (0.50 "#f39c12") (0.75 "#2ecc71") (1.0 "#3498db"))))

(add (with-transform
       (apply-shader! (rect 256 0 256 300) quantized)
       (translate 0 0)))

(add (text 128 15 "Continuous" :fill "#333" :font-size 11 :align 'center))
(add (text 384 15 "Quantized" :fill "#333" :font-size 11 :align 'center))

(render "07_quantize_noise.png")

(print "✓ 所有量化噪声示例已渲染完成")
