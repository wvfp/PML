; ═══════════════════════════════════════════════════════════════════════════════
; Rough Style — 手绘风格渲染示例
; ───────────────────────────────────────────────────────────────────────────────
; 展示 path 和 polygon 的 rough-style 渲染效果，包括 roughness、bowing、seed、
; fill-style 等参数的各种组合。
;
; 运行：pml examples\02-graphics\07_rough_style.pml
; ═══════════════════════════════════════════════════════════════════════════════

(set-backend! "skia")
(canvas 800 600 :bg "#FFFEF5")

; ── Row 1: 不同的 roughness 值 ────────────────────────────────────────────
(add (text 20 30 "Roughness 对比：" :fill "#333" :font-size 14))

; roughness = 0.5 (轻微手绘)
(add (path (list (list 'move-to 50 60)
                 (list 'line-to 150 60)
                 (list 'line-to 150 140)
                 (list 'line-to 50 140)
                 (list 'close))
          :fill "#95a5a6" :stroke "#7f8c8d" :stroke-width 2 :roughness 0.5 :seed 1))
(add (text 60 160 "r=0.5" :fill "#666" :font-size 11))

; roughness = 1.5 (中等)
(add (path (list (list 'move-to 200 60)
                 (list 'line-to 300 60)
                 (list 'line-to 300 140)
                 (list 'line-to 200 140)
                 (list 'close))
          :fill "#3498db" :stroke "#2980b9" :stroke-width 2 :roughness 1.5 :seed 42))
(add (text 210 160 "r=1.5" :fill "#666" :font-size 11))

; roughness = 3.0 (强烈手绘)
(add (path (list (list 'move-to 350 60)
                 (list 'line-to 450 60)
                 (list 'line-to 450 140)
                 (list 'line-to 350 140)
                 (list 'close))
          :fill "#e74c3c" :stroke "#c0392b" :stroke-width 2 :roughness 3.0 :seed 123))
(add (text 360 160 "r=3.0" :fill "#666" :font-size 11))

; roughness = 5.0 (极强)
(add (path (list (list 'move-to 500 60)
                 (list 'line-to 600 60)
                 (list 'line-to 600 140)
                 (list 'line-to 500 140)
                 (list 'close))
          :fill "#2ecc71" :stroke "#27ae60" :stroke-width 2 :roughness 5.0 :seed 999))
(add (text 510 160 "r=5.0" :fill "#666" :font-size 11))

; ── Row 2: 相同 roughness 不同 seed（随机变化）───────────────────────────
(add (text 20 190 "相同 roughness，不同 seed：" :fill "#333" :font-size 14))

(add (path (list (list 'move-to 50 210)
                 (list 'cubic-to 90 180 110 250 150 210)
                 (list 'cubic-to 190 170 210 250 230 210))
          :fill "none" :stroke "#9b59b6" :stroke-width 2.5 :roughness 2.0 :seed 1))
(add (text 90 250 "seed=1" :fill "#666" :font-size 11))

(add (path (list (list 'move-to 280 210)
                 (list 'cubic-to 320 180 340 250 380 210)
                 (list 'cubic-to 420 170 440 250 460 210))
          :fill "none" :stroke "#9b59b6" :stroke-width 2.5 :roughness 2.0 :seed 42))
(add (text 330 250 "seed=42" :fill "#666" :font-size 11))

(add (path (list (list 'move-to 510 210)
                 (list 'cubic-to 550 180 570 250 610 210)
                 (list 'cubic-to 650 170 670 250 690 210))
          :fill "none" :stroke "#9b59b6" :stroke-width 2.5 :roughness 2.0 :seed 123))
(add (text 560 250 "seed=123" :fill "#666" :font-size 11))

; ── Row 3: 有填充的 rough path ───────────────────────────────────────────
(add (text 20 280 "有填充的 rough path：" :fill "#333" :font-size 14))

; 心形 rough path
(add (path :d "M 100 330 C 100 290, 140 260, 170 290 C 200 260, 240 290, 240 330 C 240 380, 170 430, 170 430 C 170 430, 100 380, 100 330 Z"
          :fill "#FF6B6B" :stroke "#C92A2A" :stroke-width 2 :roughness 1.5 :seed 77))

; 星星 rough path
(add (path :d "M 350 290 L 370 350 L 430 350 L 385 390 L 405 450 L 350 410 L 295 450 L 315 390 L 270 350 L 330 350 Z"
          :fill "#FFD43B" :stroke "#F08C00" :stroke-width 2 :roughness 2.0 :seed 55))

; 圆形 rough path
(add (path (list (list 'move-to 570 370)
                 (list 'cubic-to 620 320 720 320 770 370)
                 (list 'cubic-to 820 420 720 470 570 470)
                 (list 'cubic-to 520 470 420 420 570 370)
                 (list 'close))
          :fill "#74C0FC" :stroke "#1864AB" :stroke-width 2 :roughness 2.5 :seed 33))

; ── Row 4: bowing 参数效果 ────────────────────────────────────────────────
(add (text 20 490 "Bowing（弯曲）参数：" :fill "#333" :font-size 14))

; bowing = 0 (无弯曲)
(add (path (list (list 'move-to 50 510)
                 (list 'line-to 150 510)
                 (list 'line-to 150 590)
                 (list 'line-to 50 590)
                 (list 'close))
          :fill "#e67e22" :stroke "#d35400" :stroke-width 2 :roughness 1.5 :bowing 0 :seed 1))
(add (text 60 610 "b=0" :fill "#666" :font-size 11))

; bowing = 1 (标准弯曲)
(add (path (list (list 'move-to 200 510)
                 (list 'line-to 300 510)
                 (list 'line-to 300 590)
                 (list 'line-to 200 590)
                 (list 'close))
          :fill "#e67e22" :stroke "#d35400" :stroke-width 2 :roughness 1.5 :bowing 1 :seed 1))
(add (text 210 610 "b=1" :fill "#666" :font-size 11))

; bowing = 3 (强弯曲)
(add (path (list (list 'move-to 350 510)
                 (list 'line-to 450 510)
                 (list 'line-to 450 590)
                 (list 'line-to 350 590)
                 (list 'close))
          :fill "#e67e22" :stroke "#d35400" :stroke-width 2 :roughness 1.5 :bowing 3 :seed 1))
(add (text 360 610 "b=3" :fill "#666" :font-size 11))

; bowing = 5 (极强弯曲)
(add (path (list (list 'move-to 500 510)
                 (list 'line-to 600 510)
                 (list 'line-to 600 590)
                 (list 'line-to 500 590)
                 (list 'close))
          :fill "#e67e22" :stroke "#d35400" :stroke-width 2 :roughness 1.5 :bowing 5 :seed 1))
(add (text 510 610 "b=5" :fill "#666" :font-size 11))

(render "07_rough_style.png")
