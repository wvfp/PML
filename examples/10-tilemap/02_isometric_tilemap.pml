; 示例 2：等距 Tilemap — Painter's Algorithm 深度排序
; 展示等距投影瓦片地图，含草地/水域/沙地/石头和装饰细节
;
; 运行：pml.exe examples/10-tilemap/02_isometric_tilemap.pml
; 输出：02_isometric_tilemap.png（输出到当前工作目录）

(set-backend! "skia")

; ══════════════════════════════════════════════════════════════════════════════
; 1. 定义 tileset — 含基础图形 + 细节叠加层
; ═══════════════════════════════════════════════════════════════════════════════

(define-tileset 'iso-terrain :tile-size 64
  :tiles '(
    ; 草地 — 菱形基底 + 细节草叶
    (1 grass
      (polygon (list (list 32 0) (list 64 16) (list 32 32) (list 0 16))
               :fill "#5DAE3B" :stroke "#3D7A25" :stroke-width 1)
      (rect 28 6 8 8 :fill "#7BCF4F" :fill-opacity 0.6))

    ; 水域 — 菱形水块 + 反光亮点
    (2 water
      (polygon (list (list 32 0) (list 64 16) (list 32 32) (list 0 16))
               :fill "#3A7BD5" :stroke "#2A5AA5" :stroke-width 1)
      (ellipse 32 16 6 3 :fill "#80B5FF" :fill-opacity 0.4))

    ; 沙地 — 菱形沙块
    (3 sand
      (polygon (list (list 32 0) (list 64 16) (list 32 32) (list 0 16))
               :fill "#D4B96A" :stroke "#A88F4A" :stroke-width 1))

    ; 石头 — 菱形石块
    (4 stone
      (polygon (list (list 32 0) (list 64 16) (list 32 32) (list 0 16))
               :fill "#8A8A8A" :stroke "#555" :stroke-width 1))

    ; 泥土 — 菱形地块
    (5 dirt
      (polygon (list (list 32 0) (list 64 16) (list 32 32) (list 0 16))
               :fill "#B8924A" :stroke "#8B6B2E" :stroke-width 1))
  ))

; ══════════════════════════════════════════════════════════════════════════════
; 2. 创建等距 tilemap — 8x8，单层
; ═══════════════════════════════════════════════════════════════════════════════

(define isoTM (make-tilemap 'iso-terrain 8 8 :projection 'isometric :layers 1))

; ══════════════════════════════════════════════════════════════════════════════
; 3. 填充地图数据 — 从地图数组布局
; ═══════════════════════════════════════════════════════════════════════════════
;  地图 (0=空, 1=草地, 2=水, 3=沙, 4=石头, 5=泥土)
;
;       0  1  2  3  4  5  6  7
;   0:  .  .  .  .  .  .  .  .
;   1:  .  1  1  1  1  .  .  .
;   2:  .  1  1  1  1  2  2  .
;   3:  .  1  1  1  1  2  2  .
;   4:  .  1  1  1  1  1  .  .
;   5:  .  .  .  3  3  3  .  .
;   6:  .  .  .  3  4  4  .  .
;   7:  .  .  .  .  .  .  .  .

(tilemap-set! isoTM 0 1 1 1) (tilemap-set! isoTM 0 2 1 1)
(tilemap-set! isoTM 0 3 1 1) (tilemap-set! isoTM 0 4 1 1)

(tilemap-set! isoTM 0 1 2 1) (tilemap-set! isoTM 0 2 2 1)
(tilemap-set! isoTM 0 3 2 1) (tilemap-set! isoTM 0 4 2 1)
(tilemap-set! isoTM 0 5 2 2) (tilemap-set! isoTM 0 6 2 2)

(tilemap-set! isoTM 0 1 3 1) (tilemap-set! isoTM 0 2 3 1)
(tilemap-set! isoTM 0 3 3 1) (tilemap-set! isoTM 0 4 3 1)
(tilemap-set! isoTM 0 5 3 2) (tilemap-set! isoTM 0 6 3 2)

(tilemap-set! isoTM 0 1 4 1) (tilemap-set! isoTM 0 2 4 1)
(tilemap-set! isoTM 0 3 4 1) (tilemap-set! isoTM 0 4 4 1)
(tilemap-set! isoTM 0 5 4 1)

(tilemap-set! isoTM 0 3 5 3) (tilemap-set! isoTM 0 4 5 3) (tilemap-set! isoTM 0 5 5 3)

(tilemap-set! isoTM 0 3 6 3) (tilemap-set! isoTM 0 4 6 4) (tilemap-set! isoTM 0 5 6 4)

; ══════════════════════════════════════════════════════════════════════════════
; 4. 渲染等距 tilemap — Painter's Algorithm 自动深度排序
; ═══════════════════════════════════════════════════════════════════════════════
; isometric 投影自动按 (row+col) 从远到近排序

(render-tilemap isoTM :projection 'isometric :output "02_isometric_tilemap.png" :bg "#2C3E50")

; 等距输出尺寸 = (cols+rows)*tile_size/2 = 512x512
