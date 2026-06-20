; decorations.pml - 等距地图装饰物
; 提供：树木、石头、花朵、蘑菇等装饰元素

(provide
  draw-tree
  draw-rock
  draw-flower
  draw-mushroom
  draw-bush
  draw-crate
  draw-fence)

; ═══════════════════════════════════════════════════════════════════════════════
; 树木
; ═══════════════════════════════════════════════════════════════════════════════

; 绘本风松树（云朵状树冠、粗树干、地面阴影）
(define (draw-tree cx cy)
  (group
    ; 地面阴影
    (ellipse cx (+ cy 6) 28 8 :fill "#1B5E2040" :stroke "none")
    ; 树干
    (rect (- cx 5) (- cy 8) 10 22 :fill "#8D6E63" :stroke "#5D4037" :stroke-width 1.5)
    ; 树根两侧
    (polygon (list (list (- cx 5) (+ cy 10)) (list (- cx 9) (+ cy 14)) (list (- cx 2) (+ cy 12)))
             :fill "#8D6E63" :stroke "#5D4037" :stroke-width 1)
    (polygon (list (list (+ cx 5) (+ cy 10)) (list (+ cx 9) (+ cy 14)) (list (+ cx 2) (+ cy 12)))
             :fill "#8D6E63" :stroke "#5D4037" :stroke-width 1)
    ; 树冠：多层圆润团块（绘本感）
    (circle (- cx 14) (- cy 8) 12 :fill "#2E7D32" :stroke "#1B5E20" :stroke-width 1.2)
    (circle (+ cx 14) (- cy 8) 12 :fill "#2E7D32" :stroke "#1B5E20" :stroke-width 1.2)
    (circle cx (- cy 28) 14 :fill "#388E3C" :stroke "#1B5E20" :stroke-width 1.2)
    (circle (- cx 8) (- cy 20) 11 :fill "#43A047" :stroke "#1B5E20" :stroke-width 1.2)
    (circle (+ cx 8) (- cy 20) 11 :fill "#43A047" :stroke "#1B5E20" :stroke-width 1.2)
    (circle cx (- cy 40) 10 :fill "#66BB6A" :stroke "#1B5E20" :stroke-width 1.2)
    ; 树冠高光
    (circle (- cx 4) (- cy 34) 3 :fill "#A5D6A7" :stroke "none")
    (circle (+ cx 3) (- cy 24) 2.5 :fill "#A5D6A7" :stroke "none")))

; ═══════════════════════════════════════════════════════════════════════════════
; 石头
; ═══════════════════════════════════════════════════════════════════════════════

; 绘本风石头（圆润、有裂纹和苔藓斑点）
(define (draw-rock cx cy)
  (group
    ; 地面阴影
    (ellipse cx (+ cy 10) 24 7 :fill "#37474F33" :stroke "none")
    ; 主体：圆润的等距石块
    (polygon (list
               (list (- cx 14) (+ cy 2))
               (list (- cx 6) (- cy 8))
               (list (+ cx 8) (- cy 10))
               (list (+ cx 16) (+ cy 2))
               (list cx (+ cy 12)))
             :fill "#90A4AE" :stroke "#546E7A" :stroke-width 1.2)
    ; 高光面
    (polygon (list
               (list (- cx 6) (- cy 8))
               (list (+ cx 8) (- cy 10))
               (list (+ cx 3) (- cy 2))
               (list (- cx 10) cy))
             :fill "#B0BEC5" :stroke "none")
    ; 裂纹
    (line (- cx 2) cy (+ cx 4) (+ cy 4) :stroke "#455A64" :stroke-width 0.8)
    ; 苔藓斑点
    (circle (- cx 6) (+ cy 2) 2.5 :fill "#7CB342CC" :stroke "none")
    (circle (+ cx 8) (- cy 4) 2 :fill "#7CB34299" :stroke "none")))

; ═══════════════════════════════════════════════════════════════════════════════
; 花朵
; ═══════════════════════════════════════════════════════════════════════════════

; 绘本风小花（五瓣花、弯曲茎叶）
(define (draw-flower cx cy color)
  (group
    ; 地面阴影
    (ellipse cx (+ cy 2) 8 3 :fill "#1B5E2026" :stroke "none")
    ; 弯曲的茎
    (polygon (list
               (list cx cy)
               (list (+ cx 1) (- cy 6))
               (list (- cx 1) (- cy 12))
               (list (- cx 3) (- cy 6)))
             :fill "#66BB6A" :stroke "#388E3C" :stroke-width 0.8)
    ; 小叶子
    (ellipse (- cx 4) (- cy 5) 4 2 :fill "#66BB6A" :stroke "#388E3C" :stroke-width 0.5)
    ; 五片花瓣（椭圆）
    (ellipse cx (- cy 18) 3.5 5 :fill color :stroke "#333" :stroke-width 0.6)
    (ellipse (- cx 4) (- cy 15) 3.5 5 :fill color :stroke "#333" :stroke-width 0.6)
    (ellipse (+ cx 4) (- cy 15) 3.5 5 :fill color :stroke "#333" :stroke-width 0.6)
    (ellipse (- cx 2.5) (- cy 11) 3.5 5 :fill color :stroke "#333" :stroke-width 0.6)
    (ellipse (+ cx 2.5) (- cy 11) 3.5 5 :fill color :stroke "#333" :stroke-width 0.6)
    ; 花心
    (circle cx (- cy 14) 3 :fill "#FFD54F" :stroke "#333" :stroke-width 0.5)))

; ═══════════════════════════════════════════════════════════════════════════════
; 蘑菇
; ═══════════════════════════════════════════════════════════════════════════════

; 绘本风蘑菇（圆润菌盖、自然斑点、阴影）
(define (draw-mushroom cx cy)
  (group
    ; 地面阴影
    (ellipse cx (+ cy 8) 16 5 :fill "#3E272333" :stroke "none")
    ; 菌柄
    (polygon (list
               (list (- cx 4) (+ cy 6))
               (list (- cx 3) (- cy 2))
               (list (+ cx 3) (- cy 2))
               (list (+ cx 4) (+ cy 6)))
             :fill "#FFF8E1" :stroke "#D7CCC8" :stroke-width 1)
    ; 菌盖（圆润半圆）
    (polygon (list
               (list (- cx 12) (- cy 2))
               (list (- cx 8) (- cy 12))
               (list cx (- cy 16))
               (list (+ cx 8) (- cy 12))
               (list (+ cx 12) (- cy 2)))
             :fill "#E53935" :stroke "#B71C1C" :stroke-width 1.2)
    ; 菌盖内侧阴影
    (polygon (list
               (list (- cx 10) (- cy 2))
               (list cx (- cy 8))
               (list (+ cx 10) (- cy 2)))
             :fill "#EF9A9A" :stroke "none")
    ; 菌盖白点（不规则大小）
    (circle (- cx 5) (- cy 10) 2.2 :fill "#FFFFFF" :stroke "#B71C1C" :stroke-width 0.4)
    (circle (+ cx 4) (- cy 7) 1.6 :fill "#FFFFFF" :stroke "#B71C1C" :stroke-width 0.4)
    (circle cx (- cy 12) 1.3 :fill "#FFFFFF" :stroke "#B71C1C" :stroke-width 0.4)))

; ═══════════════════════════════════════════════════════════════════════════════
; 灌木
; ═══════════════════════════════════════════════════════════════════════════════

; 绘本风灌木丛（多层圆润团块、高光、小浆果）
(define (draw-bush cx cy)
  (group
    ; 地面阴影
    (ellipse cx (+ cy 8) 26 7 :fill "#1B5E2033" :stroke "none")
    ; 后层大团
    (circle (- cx 8) (+ cy 2) 9 :fill "#2E7D32" :stroke "#1B5E20" :stroke-width 1)
    (circle (+ cx 8) (+ cy 2) 9 :fill "#2E7D32" :stroke "#1B5E20" :stroke-width 1)
    (circle cx (- cy 8) 11 :fill "#388E3C" :stroke "#1B5E20" :stroke-width 1)
    ; 中层
    (circle (- cx 4) cy 7 :fill "#43A047" :stroke "#1B5E20" :stroke-width 1)
    (circle (+ cx 4) cy 7 :fill "#43A047" :stroke "#1B5E20" :stroke-width 1)
    ; 前层高光
    (circle cx (- cy 10) 5 :fill "#66BB6A" :stroke "#1B5E20" :stroke-width 0.8)
    ; 小浆果
    (circle (+ cx 6) (- cy 4) 1.8 :fill "#E53935" :stroke "#B71C1C" :stroke-width 0.4)
    (circle (- cx 5) (+ cy 2) 1.5 :fill "#E53935" :stroke "#B71C1C" :stroke-width 0.4)))

; ═══════════════════════════════════════════════════════════════════════════════
; 箱子
; ═══════════════════════════════════════════════════════════════════════════════

; 绘本风木箱（木纹、钉子、柔和阴影）
(define (draw-crate cx cy)
  (group
    ; 地面阴影
    (ellipse cx (+ cy 14) 28 8 :fill "#3E272333" :stroke "none")
    ; 左侧面
    (polygon (list
               (list (- cx 12) (- cy 6))
               (list cx cy)
               (list cx (+ cy 12))
               (list (- cx 12) (+ cy 6)))
             :fill "#A1887F" :stroke "#5D4037" :stroke-width 1.2)
    ; 右侧面
    (polygon (list
               (list (+ cx 12) (- cy 6))
               (list cx cy)
               (list cx (+ cy 12))
               (list (+ cx 12) (+ cy 6)))
             :fill "#8D6E63" :stroke "#5D4037" :stroke-width 1.2)
    ; 顶面
    (polygon (list
               (list cx (- cy 12))
               (list (+ cx 12) (- cy 6))
               (list cx cy)
               (list (- cx 12) (- cy 6)))
             :fill "#D7CCC8" :stroke "#5D4037" :stroke-width 1.2)
    ; 木纹
    (line (- cx 8) (- cy 8) (- cx 2) (- cy 4) :stroke "#8D6E63" :stroke-width 0.6)
    (line (+ cx 2) (- cy 8) (+ cx 8) (- cy 4) :stroke "#8D6E63" :stroke-width 0.6)
    ; 十字绑带
    (line (- cx 6) (- cy 9) (+ cx 6) (- cy 3) :stroke "#5D4037" :stroke-width 2)
    (line (+ cx 6) (- cy 9) (- cx 6) (- cy 3) :stroke "#5D4037" :stroke-width 2)
    ; 钉子
    (circle (- cx 6) (- cy 9) 1 :fill "#3E2723" :stroke "none")
    (circle (+ cx 6) (- cy 3) 1 :fill "#3E2723" :stroke "none")
    (circle (+ cx 6) (- cy 9) 1 :fill "#3E2723" :stroke "none")
    (circle (- cx 6) (- cy 3) 1 :fill "#3E2723" :stroke "none")))

; ═══════════════════════════════════════════════════════════════════════════════
; 栅栏
; ═══════════════════════════════════════════════════════════════════════════════

; 绘本风栅栏（木柱有顶、钉子、自然木纹）
(define (draw-fence cx cy)
  (group
    ; 地面阴影
    (ellipse cx (+ cy 4) 34 6 :fill "#3E272326" :stroke "none")
    ; 左柱
    (rect (- cx 14) (- cy 18) 5 22 :fill "#A1887F" :stroke "#5D4037" :stroke-width 1.2)
    ; 左柱顶
    (polygon (list
               (list (- cx 16) (- cy 18))
               (list (- cx 11.5) (- cy 22))
               (list (- cx 9) (- cy 18)))
             :fill "#D7CCC8" :stroke "#5D4037" :stroke-width 1)
    ; 右柱
    (rect (+ cx 9) (- cy 18) 5 22 :fill "#8D6E63" :stroke "#5D4037" :stroke-width 1.2)
    ; 右柱顶
    (polygon (list
               (list (+ cx 11) (- cy 18))
               (list (+ cx 15.5) (- cy 22))
               (list (+ cx 20) (- cy 18)))
             :fill "#D7CCC8" :stroke "#5D4037" :stroke-width 1)
    ; 横杆上
    (polygon (list
               (list (- cx 12) (- cy 14))
               (list (+ cx 12) (- cy 14))
               (list (+ cx 12) (- cy 11))
               (list (- cx 12) (- cy 11)))
             :fill "#D7CCC8" :stroke "#5D4037" :stroke-width 1)
    ; 横杆下
    (polygon (list
               (list (- cx 12) (- cy 7))
               (list (+ cx 12) (- cy 7))
               (list (+ cx 12) (- cy 4))
               (list (- cx 12) (- cy 4)))
             :fill "#BCAAA4" :stroke "#5D4037" :stroke-width 1)
    ; 钉子
    (circle (- cx 12) (- cy 12.5) 1 :fill "#3E2723" :stroke "none")
    (circle (+ cx 12) (- cy 12.5) 1 :fill "#3E2723" :stroke "none")
    (circle (- cx 12) (- cy 5.5) 1 :fill "#3E2723" :stroke "none")
    (circle (+ cx 12) (- cy 5.5) 1 :fill "#3E2723" :stroke "none")))
