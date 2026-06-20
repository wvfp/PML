; 项目 1：视差卷轴平台场景
; 多层背景、瓦片地面、树木与角色，展示模块导入与组合。

(set-backend! "skia")
(canvas 640 360 :bg "#87CEEB")

(import "tiles.pml" as t)

; 太阳
(add (circle 540 80 45 :fill "#FFD700" :stroke "#FFA500" :stroke-width 3))

; 远山（视差层 1）
(add (polygon (list '(0 360) '(160 240) '(320 300) '(480 220) '(640 280) '(640 360))
              :fill "#5D8AA8" :stroke "#4A708B" :stroke-width 1))

; 中景山丘（视差层 2）
(add (polygon (list '(0 360) '(120 290) '(280 330) '(440 270) '(640 320) '(640 360))
              :fill "#4A7C59" :stroke "#3E6B4A" :stroke-width 1))

; 地面瓦片
(define (ground-row y count size start-x)
  (if (> count 0)
      (begin
        (add (t/draw-ground-tile start-x y size))
        (ground-row y (- count 1) size (+ start-x size)))))

(ground-row 320 11 64 -32)

; 树木
(add (t/draw-tree 110 320 1.3))
(add (t/draw-tree 520 320 1.0))

; 角色
(define hero (character :expression 'happy :style 'cel :scale 0.9))
(add (translate-object hero 300 300))

; 云朵
(add (ellipse 150 90 60 30 :fill "#FFFFFF" :opacity 0.8))
(add (ellipse 180 100 50 25 :fill "#FFFFFF" :opacity 0.8))
(add (ellipse 420 70 70 35 :fill "#FFFFFF" :opacity 0.8))

(add (text 320 30 "Parallax Platformer" :fill "#FFFFFF" :font-size 24 :align 'center))

(render "01_parallax_platformer.png")
