; 示例 3：渲染通道 — 精灵多通道预烘焙
; 展示 render-channels 输出 albedo / normal / specular 三通道
;
; 运行：pml.exe examples/10-tilemap/03_render_channels.pml
; 输出：sprite_sword_albedo.png / sprite_sword_normal.png / sprite_sword_specular.png（输出到当前工作目录）

(set-backend! "skia")

; ══════════════════════════════════════════════════════════════════════════════
; 构建一个组合精灵 — 像素风格长剑
; ═══════════════════════════════════════════════════════════════════════════════

(define sword
  (group
    ; 剑刃 — 亮金属
    (rect 28 0 8 40 :fill "#C0C0C0" :stroke "#808080" :stroke-width 1)
    ; 剑刃高光
    (rect 30 2 4 36 :fill "#E8E8E8" :fill-opacity 0.5)
    ; 剑格（护手）
    (rect 20 40 24 6 :fill "#8B4513" :stroke "#5C2E0A" :stroke-width 1)
    ; 剑格铆钉
    (circle 26 43 2 :fill "#FFD700")
    (circle 38 43 2 :fill "#FFD700")
    ; 剑柄
    (rect 26 46 12 18 :fill "#5C2E0A" :stroke "#3A1D05" :stroke-width 1)
    ; 剑柄缠绳纹理
    (line 28 50 36 50 :stroke "#8B4513" :stroke-width 1)
    (line 28 54 36 54 :stroke "#8B4513" :stroke-width 1)
    (line 28 58 36 58 :stroke "#8B4513" :stroke-width 1)
    ; 柄尾配重
    (rect 30 64 4 6 :fill "#FFD700" :stroke "#CC9900" :stroke-width 1)
  ))

; ══════════════════════════════════════════════════════════════════════════════
; 多通道预烘焙 — 一行代码导出三通道
; ═══════════════════════════════════════════════════════════════════════════════
; (render-channels 图形对象 :output '前缀 :channels '(通道列表) [:width W] [:height H])

(define channels-out
  (render-channels sword
    :output 'sprite_sword
    :channels '(albedo specular normal)
    :width 64
    :height 80))

; channels-out = (sprite_sword_albedo.png sprite_sword_specular.png sprite_sword_normal.png)

(println "Generated channels:") 
(println "  " (car channels-out))
(println "  " (car (cdr channels-out)))
(println "  " (car (cdr (cdr channels-out))))

; ══════════════════════════════════════════════════════════════════════════════
; 通道说明：
; ═══════════════════════════════════════════════════════════════════════════════
; albedo.png    = 原始颜色渲染（漫反射贴图）
; specular.png  = 灰度亮度值（高光贴图）
; normal.png    = 默认法线 RGB(128,128,255)（法线贴图，面向 Z 轴）
;
; 游戏引擎用法：
;   - albedo   → 基础颜色纹理
;   - specular → 高光强度（白色=全高光，黑色=无高光）
;   - normal   → 法线贴图（需在引擎中转为 tangent-space）
