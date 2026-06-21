; 示例 1：正交 Tilemap 多地形渲染
; 展示 define-tileset / make-tilemap / tilemap-set! / render-tilemap 的用法
;
; 运行：pml.exe examples/10-tilemap/01_orthogonal_tilemap.pml
; 输出：01_orthogonal_tilemap.png（输出到当前工作目录）

(set-backend! "skia")

; ══════════════════════════════════════════════════════════════════════════════
; 1. 定义 tileset — 注册瓦片类型
; ═══════════════════════════════════════════════════════════════════════════════

(define-tileset 'terrain :tile-size 32
  :tiles '(
    (1 grass  (rect 0 0 32 32 :fill "#6DBF47" :stroke "#4A8F2A" :stroke-width 1))
    (2 water  (rect 0 0 32 32 :fill "#4A90D9" :stroke "#2C6BA0" :stroke-width 1))
    (3 sand   (rect 0 0 32 32 :fill "#E8D68C" :stroke "#BFA86A" :stroke-width 1))
    (4 stone  (rect 0 0 32 32 :fill "#9E9E9E" :stroke "#666" :stroke-width 1))
    (5 dirt   (rect 0 0 32 32 :fill "#C4A265" :stroke "#8B7355" :stroke-width 1))
    (6 path   (rect 0 0 32 32 :fill "#D4A76A" :stroke "#B8925A" :stroke-width 1))
  ))

; ══════════════════════════════════════════════════════════════════════════════
; 2. 创建 tilemap — 12x10 正交，2 层
; ═══════════════════════════════════════════════════════════════════════════════
; (make-tilemap tileset-name cols rows :projection 'orthogonal|'isometric :layers N)

(define tm (make-tilemap 'terrain 12 10 :projection 'orthogonal :layers 2))

; ══════════════════════════════════════════════════════════════════════════════
; 3. 填充 tile — 第 0 层：地形基底
; ═══════════════════════════════════════════════════════════════════════════════

; 左上池塘 (id=2)
(tilemap-set! tm 0 1 0 2) (tilemap-set! tm 0 2 0 2)
(tilemap-set! tm 0 1 1 2) (tilemap-set! tm 0 2 1 2)
(tilemap-set! tm 0 0 1 2) (tilemap-set! tm 0 0 2 2)

; 草地 (id=1) — 中间大块区域
(tilemap-set! tm 0 3 0 1) (tilemap-set! tm 0 4 0 1) (tilemap-set! tm 0 5 0 1)
(tilemap-set! tm 0 3 1 1) (tilemap-set! tm 0 4 1 1) (tilemap-set! tm 0 5 1 1)
(tilemap-set! tm 0 3 2 1) (tilemap-set! tm 0 4 2 1) (tilemap-set! tm 0 5 2 1)
(tilemap-set! tm 0 1 3 1) (tilemap-set! tm 0 2 3 1) (tilemap-set! tm 0 3 3 1)
(tilemap-set! tm 0 4 3 1) (tilemap-set! tm 0 5 3 1) (tilemap-set! tm 0 6 3 1)
(tilemap-set! tm 0 1 4 1) (tilemap-set! tm 0 2 4 1) (tilemap-set! tm 0 3 4 1)
(tilemap-set! tm 0 4 4 1) (tilemap-set! tm 0 5 4 1) (tilemap-set! tm 0 6 4 1)

; 右下沙地 (id=3)
(tilemap-set! tm 0 9 3 3) (tilemap-set! tm 0 10 3 3) (tilemap-set! tm 0 11 3 3)
(tilemap-set! tm 0 9 4 3) (tilemap-set! tm 0 10 4 3) (tilemap-set! tm 0 11 4 3)
(tilemap-set! tm 0 9 5 3) (tilemap-set! tm 0 10 5 3) (tilemap-set! tm 0 11 5 3)

; 石区 (id=4)
(tilemap-set! tm 0 7 6 4) (tilemap-set! tm 0 8 6 4) (tilemap-set! tm 0 9 6 4)
(tilemap-set! tm 0 7 7 4) (tilemap-set! tm 0 8 7 4) (tilemap-set! tm 0 9 7 4)

; 泥土 (id=5)
(tilemap-set! tm 0 4 8 5) (tilemap-set! tm 0 5 8 5) (tilemap-set! tm 0 6 8 5)
(tilemap-set! tm 0 4 9 5) (tilemap-set! tm 0 5 9 5) (tilemap-set! tm 0 6 9 5)

; 更多草地
(tilemap-set! tm 0 0 3 1) (tilemap-set! tm 0 0 4 1)
(tilemap-set! tm 0 0 5 1) (tilemap-set! tm 0 1 5 1) (tilemap-set! tm 0 2 5 1)
(tilemap-set! tm 0 7 3 1) (tilemap-set! tm 0 8 3 1)
(tilemap-set! tm 0 7 4 1) (tilemap-set! tm 0 8 4 1)
(tilemap-set! tm 0 3 6 1) (tilemap-set! tm 0 4 6 1) (tilemap-set! tm 0 5 6 1)
(tilemap-set! tm 0 6 6 1) (tilemap-set! tm 0 10 6 1) (tilemap-set! tm 0 11 6 1)
(tilemap-set! tm 0 3 7 1) (tilemap-set! tm 0 4 7 1) (tilemap-set! tm 0 5 7 1)
(tilemap-set! tm 0 6 7 1) (tilemap-set! tm 0 10 7 1) (tilemap-set! tm 0 11 7 1)
(tilemap-set! tm 0 0 6 1) (tilemap-set! tm 0 1 6 1) (tilemap-set! tm 0 2 6 1)
(tilemap-set! tm 0 0 7 1) (tilemap-set! tm 0 1 7 1) (tilemap-set! tm 0 2 7 1)
(tilemap-set! tm 0 7 8 1) (tilemap-set! tm 0 8 8 1) (tilemap-set! tm 0 9 8 1)
(tilemap-set! tm 0 10 8 1) (tilemap-set! tm 0 11 8 1)
(tilemap-set! tm 0 7 9 1) (tilemap-set! tm 0 8 9 1) (tilemap-set! tm 0 9 9 1)
(tilemap-set! tm 0 10 9 1) (tilemap-set! tm 0 11 9 1)

; ══════════════════════════════════════════════════════════════════════════════
; 第 1 层：路径覆盖层 (id=6)
; ═══════════════════════════════════════════════════════════════════════════════
(tilemap-set! tm 1 1 5 6) (tilemap-set! tm 1 2 5 6) (tilemap-set! tm 1 3 5 6)
(tilemap-set! tm 1 3 6 6) (tilemap-set! tm 1 3 7 6) (tilemap-set! tm 1 4 7 6)
(tilemap-set! tm 1 5 7 6) (tilemap-set! tm 1 6 7 6) (tilemap-set! tm 1 6 6 6)
(tilemap-set! tm 1 6 5 6) (tilemap-set! tm 1 5 5 6) (tilemap-set! tm 1 4 5 6)

; ══════════════════════════════════════════════════════════════════════════════
; 4. 渲染正交 tilemap
; ═══════════════════════════════════════════════════════════════════════════════
; (render-tilemap tilemap-name :output "file.png" [:bg "color"])

(render-tilemap tm :output "01_orthogonal_tilemap.png" :bg "#1a1a2e")
