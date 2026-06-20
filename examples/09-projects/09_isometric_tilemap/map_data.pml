; map_data.pml - 地图数据定义
; 瓦片类型编码：
;   0 = 草地  1 = 水面  2 = 沙地
;   3 = 路径  4 = 石头  5 = 泥土

(provide
  map-grid
  map-width
  map-height
  decorations-list)

; ══════════════════════════════════════════════════════════════════════════════
; 地图网格（10x10）
; ═══════════════════════════════════════════════════════════════════════════════

(define map-grid
  (list
    ; 行 0
    (list 0 0 0 0 0 0 0 0 0 0)
    ; 行 1
    (list 0 0 0 0 0 0 0 0 0 0)
    ; 行 2 - 水面区域
    (list 0 0 1 1 1 0 0 0 0 0)
    ; 行 3
    (list 0 0 1 1 1 1 0 0 0 0)
    ; 行 4 - 沙地环绕水面
    (list 0 0 2 1 1 2 0 0 0 0)
    ; 行 5 - 路径
    (list 0 0 0 2 2 0 0 3 3 0)
    ; 行 6
    (list 0 0 0 0 0 0 3 0 0 0)
    ; 行 7 - 石头区域
    (list 0 0 0 0 0 0 3 0 4 4)
    ; 行 8
    (list 0 0 5 5 0 0 3 3 0 0)
    ; 行 9
    (list 0 0 0 0 0 0 0 0 0 0)))

(define map-width 10)
(define map-height 10)

; ═══════════════════════════════════════════════════════════════════════════════
; 装饰物列表
; 格式: (list type col row)
; type: 'tree 'rock 'flower 'mushroom 'bush 'crate 'fence
; ═══════════════════════════════════════════════════════════════════════════════

(define decorations-list
  (list
    ; 树木
    (list 'tree 1 1)
    (list 'tree 8 1)
    (list 'tree 0 5)
    (list 'tree 9 3)
    (list 'tree 7 8)
    (list 'tree 2 8)
    (list 'tree 5 0)
    ; 石头
    (list 'rock 8 7)
    (list 'rock 9 8)
    (list 'rock 7 9)
    ; 花朵
    (list 'flower 3 0 "#FF6B6B")
    (list 'flower 6 2 "#FFD93D")
    (list 'flower 4 9 "#C084FC")
    (list 'flower 1 7 "#FF6B6B")
    ; 蘑菇
    (list 'mushroom 6 0)
    (list 'mushroom 3 9)
    ; 灌木
    (list 'bush 0 8)
    (list 'bush 9 6)
    ; 箱子
    (list 'crate 6 6)
    ; 栅栏
    (list 'fence 5 5)))
