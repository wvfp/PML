; decorations.pml - 等距地图装饰物（Stardew Valley 风格）

(provide
  draw-tree
  draw-tree-variant
  draw-rock
  draw-flower
  draw-mushroom
  draw-bush
  draw-crate
  draw-fence)

; 更柔和、低饱和、带手绘细节

(define (draw-tree cx cy)
  (draw-tree-variant cx cy 'oak))

(define (draw-tree-variant cx cy kind)
  (cond
    ((eq? kind 'small) (draw-tree-small cx cy))
    ((eq? kind 'pine) (draw-tree-pine cx cy))
    ((eq? kind 'round) (draw-tree-round cx cy))
    (else (draw-tree-oak cx cy))))

(define (draw-tree-oak cx cy)
  (group
    ; 地面阴影
    (ellipse cx (+ cy 28) 96 24 :fill "#2D4A1A60" :stroke "none")
    ; 树干：带纹理和分叉
    (polygon (list
               (list (- cx 14) (+ cy 28))
               (list (- cx 8) (- cy 24))
               (list cx (- cy 64))
               (list (+ cx 8) (- cy 24))
               (list (+ cx 14) (+ cy 28)))
             :fill "#6D4C32" :stroke "#4E342E" :stroke-width 5)
    ; 树皮纹理
    (line (- cx 4) (- cy 16) (- cx 4) (+ cy 12) :stroke "#4E342E" :stroke-width 2.4)
    (line (+ cx 4) (- cy 32) (+ cx 4) (- cy 8) :stroke "#4E342E" :stroke-width 2.4)
    ; 树冠：不规则团簇，更像橡树
    (circle (- cx 44) (- cy 44) 42 :fill "#3E6B2F" :stroke "#2E4D22" :stroke-width 4)
    (circle (+ cx 44) (- cy 44) 42 :fill "#3E6B2F" :stroke "#2E4D22" :stroke-width 4)
    (circle (- cx 26) (- cy 84) 38 :fill "#4A7F38" :stroke "#2E4D22" :stroke-width 4)
    (circle (+ cx 26) (- cy 84) 38 :fill "#4A7F38" :stroke "#2E4D22" :stroke-width 4)
    (circle cx (- cy 120) 44 :fill "#568C42" :stroke "#2E4D22" :stroke-width 4)
    ; 高光团
    (circle (- cx 18) (- cy 100) 18 :fill "#6BA85A" :stroke "#2E4D22" :stroke-width 3)
    (circle (+ cx 22) (- cy 68) 16 :fill "#6BA85A" :stroke "#2E4D22" :stroke-width 3)
    (circle cx (- cy 140) 14 :fill "#7AB86A" :stroke "none")
    ; 低处叶片
    (circle (- cx 52) (- cy 20) 20 :fill "#3E6B2F" :stroke "#2E4D22" :stroke-width 3)
    (circle (+ cx 52) (- cy 20) 20 :fill "#3E6B2F" :stroke "#2E4D22" :stroke-width 3)))

(define (draw-tree-small cx cy)
  (group
    ; 地面阴影
    (ellipse cx (+ cy 18) 64 16 :fill "#2D4A1A60" :stroke "none")
    ; 树干
    (polygon (list
               (list (- cx 8) (+ cy 18))
               (list (- cx 4) (- cy 12))
               (list cx (- cy 40))
               (list (+ cx 4) (- cy 12))
               (list (+ cx 8) (+ cy 18)))
             :fill "#6D4C32" :stroke "#4E342E" :stroke-width 3.2)
    ; 树冠
    (circle (- cx 24) (- cy 28) 26 :fill "#4A7F38" :stroke "#2E4D22" :stroke-width 3)
    (circle (+ cx 24) (- cy 28) 26 :fill "#4A7F38" :stroke "#2E4D22" :stroke-width 3)
    (circle cx (- cy 56) 30 :fill "#568C42" :stroke "#2E4D22" :stroke-width 3)
    (circle (- cx 10) (- cy 70) 10 :fill "#7AB86A" :stroke "none")
    (circle (+ cx 14) (- cy 50) 9 :fill "#7AB86A" :stroke "none")))

(define (draw-tree-pine cx cy)
  (group
    ; 地面阴影
    (ellipse cx (+ cy 22) 72 18 :fill "#2D4A1A60" :stroke "none")
    ; 树干
    (polygon (list
               (list (- cx 10) (+ cy 22))
               (list (- cx 6) (- cy 20))
               (list cx (- cy 80))
               (list (+ cx 6) (- cy 20))
               (list (+ cx 10) (+ cy 22)))
             :fill "#5D4037" :stroke "#3E2723" :stroke-width 4)
    ; 松针层次
    (polygon (list
               (list cx (- cy 120))
               (list (- cx 50) (- cy 50))
               (list (+ cx 50) (- cy 50)))
             :fill "#2E4D22" :stroke "#1E3316" :stroke-width 4)
    (polygon (list
               (list cx (- cy 90))
               (list (- cx 58) (- cy 10))
               (list (+ cx 58) (- cy 10)))
             :fill "#3E6B2F" :stroke "#1E3316" :stroke-width 4)
    (polygon (list
               (list cx (- cy 55))
               (list (- cx 62) (+ cy 30))
               (list (+ cx 62) (+ cy 30)))
             :fill "#4A7F38" :stroke "#1E3316" :stroke-width 4)
    ; 高光
    (polygon (list
               (list cx (- cy 110))
               (list (- cx 18) (- cy 65))
               (list cx (- cy 75)))
             :fill "#6BA85A" :stroke "none")
    (polygon (list
               (list cx (- cy 75))
               (list (- cx 22) (- cy 15))
               (list cx (- cy 30)))
             :fill "#6BA85A" :stroke "none")))

(define (draw-tree-round cx cy)
  (group
    ; 地面阴影
    (ellipse cx (+ cy 20) 80 20 :fill "#2D4A1A60" :stroke "none")
    ; 树干
    (polygon (list
               (list (- cx 10) (+ cy 20))
               (list (- cx 6) (- cy 16))
               (list cx (- cy 48))
               (list (+ cx 6) (- cy 16))
               (list (+ cx 10) (+ cy 20)))
             :fill "#6D4C32" :stroke "#4E342E" :stroke-width 4)
    ; 圆形树冠
    (circle cx (- cy 56) 50 :fill "#4A7F38" :stroke "#2E4D22" :stroke-width 4)
    (circle (- cx 18) (- cy 70) 28 :fill "#568C42" :stroke "#2E4D22" :stroke-width 3)
    (circle (+ cx 22) (- cy 62) 24 :fill "#568C42" :stroke "#2E4D22" :stroke-width 3)
    (circle (- cx 10) (- cy 90) 14 :fill "#7AB86A" :stroke "none")
    (circle (+ cx 14) (- cy 86) 12 :fill "#7AB86A" :stroke "none")))

(define (draw-rock cx cy)
  (group
    ; 地面阴影
    (ellipse cx (+ cy 36) 80 22 :fill "#37474F40" :stroke "none")
    ; 主体：更不规则的石头
    (polygon (list
               (list (- cx 52) (+ cy 8))
               (list (- cx 32) (- cy 28))
               (list cx (- cy 44))
               (list (+ cx 36) (- cy 24))
               (list (+ cx 56) (+ cy 12))
               (list cx (+ cy 44)))
             :fill "#7A868D" :stroke "#4A545C" :stroke-width 4.8)
    ; 高光面
    (polygon (list
               (list (- cx 32) (- cy 28))
               (list cx (- cy 44))
               (list (+ cx 8) (- cy 16))
               (list (- cx 20) cy))
             :fill "#9AA4AA" :stroke "none")
    ; 阴影面
    (polygon (list
               (list (+ cx 36) (- cy 24))
               (list (+ cx 56) (+ cy 12))
               (list cx (+ cy 44))
               (list (+ cx 8) (- cy 16)))
             :fill "#5E6970" :stroke "none")
    ; 裂纹
    (line (- cx 8) (- cy 8) (+ cx 12) (+ cy 8) :stroke "#3D4247" :stroke-width 3.2)
    (line (- cx 20) (+ cy 4) (- cx 8) (+ cy 20) :stroke "#3D4247" :stroke-width 2.4)
    ; 苔藓
    (circle (- cx 28) (+ cy 4) 9 :fill "#5E7A36AA" :stroke "none")
    (circle (+ cx 32) (- cy 12) 7 :fill "#5E7A3688" :stroke "none")))

(define (draw-flower cx cy color)
  (group
    ; 地面阴影
    (ellipse cx (+ cy 8) 28 10 :fill "#2D4A1A33" :stroke "none")
    ; 茎和叶
    (polygon (list
               (list cx cy)
               (list (+ cx 4) (- cy 24))
               (list (- cx 4) (- cy 48))
               (list (- cx 10) (- cy 20)))
             :fill "#5E8C3A" :stroke "#3E6B2F" :stroke-width 2.4)
    ; 叶子
    (ellipse (- cx 14) (- cy 16) 18 8 :fill "#6BA85A" :stroke "#3E6B2F" :stroke-width 1.6)
    (ellipse (+ cx 14) (- cy 24) 16 7 :fill "#6BA85A" :stroke "#3E6B2F" :stroke-width 1.6)
    ; 花瓣：更圆润
    (ellipse cx (- cy 62) 14 18 :fill color :stroke "#4A4A4A" :stroke-width 2)
    (ellipse (- cx 14) (- cy 52) 14 18 :fill color :stroke "#4A4A4A" :stroke-width 2)
    (ellipse (+ cx 14) (- cy 52) 14 18 :fill color :stroke "#4A4A4A" :stroke-width 2)
    (ellipse (- cx 8) (- cy 38) 14 18 :fill color :stroke "#4A4A4A" :stroke-width 2)
    (ellipse (+ cx 8) (- cy 38) 14 18 :fill color :stroke "#4A4A4A" :stroke-width 2)
    ; 花心
    (circle cx (- cy 50) 10 :fill "#FDD835" :stroke "#4A4A4A" :stroke-width 2)))

(define (draw-mushroom cx cy)
  (group
    ; 地面阴影
    (ellipse cx (+ cy 28) 56 16 :fill "#3E272340" :stroke "none")
    ; 菌柄：略弯曲
    (polygon (list
               (list (- cx 12) (+ cy 20))
               (list (- cx 8) (- cy 4))
               (list (+ cx 8) (- cy 4))
               (list (+ cx 12) (+ cy 20)))
             :fill "#F5F5DC" :stroke "#C9B99A" :stroke-width 3.2)
    ; 菌盖：更圆
    (polygon (list
               (list (- cx 40) (- cy 4))
               (list (- cx 24) (- cy 44))
               (list cx (- cy 60))
               (list (+ cx 24) (- cy 44))
               (list (+ cx 40) (- cy 4)))
             :fill "#C62828" :stroke "#8E0000" :stroke-width 4.8)
    ; 菌盖内侧
    (polygon (list
               (list (- cx 28) (- cy 4))
               (list cx (- cy 32))
               (list (+ cx 28) (- cy 4)))
             :fill "#EF9A9A" :stroke "none")
    ; 白点：大小不一
    (circle (- cx 18) (- cy 36) 7.2 :fill "#FFF8E7" :stroke "#8E0000" :stroke-width 1.4)
    (circle (+ cx 14) (- cy 24) 5.2 :fill "#FFF8E7" :stroke "#8E0000" :stroke-width 1.4)
    (circle cx (- cy 44) 4 :fill "#FFF8E7" :stroke "#8E0000" :stroke-width 1.4)
    (circle (+ cx 22) (- cy 40) 3.2 :fill "#FFF8E7" :stroke "#8E0000" :stroke-width 1.4)))

(define (draw-bush cx cy)
  (group
    ; 地面阴影
    (ellipse cx (+ cy 28) 88 22 :fill "#2D4A1A44" :stroke "none")
    ; 后层
    (circle (- cx 28) (+ cy 4) 30 :fill "#3E6B2F" :stroke "#2E4D22" :stroke-width 3.2)
    (circle (+ cx 28) (+ cy 4) 30 :fill "#3E6B2F" :stroke "#2E4D22" :stroke-width 3.2)
    (circle cx (- cy 24) 36 :fill "#4A7F38" :stroke "#2E4D22" :stroke-width 3.2)
    ; 中层
    (circle (- cx 16) (- cy 8) 24 :fill "#568C42" :stroke "#2E4D22" :stroke-width 3.2)
    (circle (+ cx 16) (- cy 8) 24 :fill "#568C42" :stroke "#2E4D22" :stroke-width 3.2)
    ; 前层高光
    (circle cx (- cy 32) 20 :fill "#6BA85A" :stroke "#2E4D22" :stroke-width 2.8)
    ; 浆果
    (circle (+ cx 22) (- cy 8) 5.6 :fill "#B71C1C" :stroke "#7F0000" :stroke-width 1.4)
    (circle (- cx 18) (+ cy 4) 4.8 :fill "#B71C1C" :stroke "#7F0000" :stroke-width 1.4)
    (circle (+ cx 8) (- cy 20) 4 :fill "#B71C1C" :stroke "#7F0000" :stroke-width 1.4)))

(define (draw-crate cx cy)
  (group
    ; 地面阴影
    (ellipse cx (+ cy 44) 96 24 :fill "#3E272344" :stroke "none")
    ; 左侧面
    (polygon (list
               (list (- cx 44) (- cy 20))
               (list cx cy)
               (list cx (+ cy 40))
               (list (- cx 44) (+ cy 20)))
             :fill "#A1887F" :stroke "#5D4037" :stroke-width 4.8)
    ; 右侧面
    (polygon (list
               (list (+ cx 44) (- cy 20))
               (list cx cy)
               (list cx (+ cy 40))
               (list (+ cx 44) (+ cy 20)))
             :fill "#8D6E63" :stroke "#5D4037" :stroke-width 4.8)
    ; 顶面
    (polygon (list
               (list cx (- cy 40))
               (list (+ cx 44) (- cy 20))
               (list cx cy)
               (list (- cx 44) (- cy 20)))
             :fill "#D7CCC8" :stroke "#5D4037" :stroke-width 4.8)
    ; 木纹
    (line (- cx 28) (- cy 28) (- cx 8) (- cy 16) :stroke "#8D6E63" :stroke-width 2.4)
    (line (+ cx 8) (- cy 28) (+ cx 28) (- cy 16) :stroke "#8D6E63" :stroke-width 2.4)
    ; 十字绑带
    (line (- cx 20) (- cy 32) (+ cx 20) (- cy 8) :stroke "#5D4037" :stroke-width 7)
    (line (+ cx 20) (- cy 32) (- cx 20) (- cy 8) :stroke "#5D4037" :stroke-width 7)
    ; 钉子
    (circle (- cx 20) (- cy 32) 3.6 :fill "#3E2723" :stroke "none")
    (circle (+ cx 20) (- cy 8) 3.6 :fill "#3E2723" :stroke "none")
    (circle (+ cx 20) (- cy 32) 3.6 :fill "#3E2723" :stroke "none")
    (circle (- cx 20) (- cy 8) 3.6 :fill "#3E2723" :stroke "none")))

(define (draw-fence cx cy)
  (group
    ; 地面阴影
    (ellipse cx (+ cy 12) 120 22 :fill "#3E272328" :stroke "none")
    ; 左柱
    (rect (- cx 52) (- cy 64) 18 76 :fill "#A1887F" :stroke "#5D4037" :stroke-width 4.8)
    ; 左柱顶
    (polygon (list
               (list (- cx 58) (- cy 64))
               (list (- cx 43) (- cy 80))
               (list (- cx 34) (- cy 64)))
             :fill "#D7CCC8" :stroke "#5D4037" :stroke-width 3.6)
    ; 右柱
    (rect (+ cx 34) (- cy 64) 18 76 :fill "#8D6E63" :stroke "#5D4037" :stroke-width 4.8)
    ; 右柱顶
    (polygon (list
               (list (+ cx 40) (- cy 64))
               (list (+ cx 55) (- cy 80))
               (list (+ cx 70) (- cy 64)))
             :fill "#D7CCC8" :stroke "#5D4037" :stroke-width 3.6)
    ; 横杆上
    (polygon (list
               (list (- cx 44) (- cy 48))
               (list (+ cx 44) (- cy 48))
               (list (+ cx 44) (- cy 36))
               (list (- cx 44) (- cy 36)))
             :fill "#D7CCC8" :stroke "#5D4037" :stroke-width 3.6)
    ; 横杆下
    (polygon (list
               (list (- cx 44) (- cy 20))
               (list (+ cx 44) (- cy 20))
               (list (+ cx 44) (- cy 8))
               (list (- cx 44) (- cy 8)))
             :fill "#BCAAA4" :stroke "#5D4037" :stroke-width 3.6)
    ; 钉子
    (circle (- cx 44) (- cy 42) 3.2 :fill "#3E2723" :stroke "none")
    (circle (+ cx 44) (- cy 42) 3.2 :fill "#3E2723" :stroke "none")
    (circle (- cx 44) (- cy 14) 3.2 :fill "#3E2723" :stroke "none")
    (circle (+ cx 44) (- cy 14) 3.2 :fill "#3E2723" :stroke "none")))
