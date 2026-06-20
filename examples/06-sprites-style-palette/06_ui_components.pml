; 示例 6：UI 组件
; 用 button、panel、health-bar、icon 拼一个游戏 HUD。

(set-backend! "skia")
(canvas 500 350 :bg "#2C3E50")

; 面板作为底
(define main-panel (panel :width 420 :height 280 :style 'simple :color "#34495E" :border-width 2))
(add (translate-object main-panel 250 175))

; 标题栏
(define title-panel (panel :width 420 :height 40 :style 'simple :color "#2980B9" :title "Status" :border-width 0))
(add (translate-object title-panel 250 35))

; 按钮
(define btn-start (button :label "START" :width 140 :height 40 :style 'rounded :color "#27AE60" :state 'normal))
(define btn-exit (button :label "EXIT" :width 100 :height 40 :style 'rounded :color "#C0392B" :state 'hover))
(add (translate-object btn-start 130 300))
(add (translate-object btn-exit 370 300))

; 血条
(define hp-bar (health-bar :value 0.72 :width 200 :height 20 :style 'gradient :color "#E74C3C" :bg-color "#1A1A1A"))
(add (translate-object hp-bar 280 110))

; 图标
(define heart-icon (icon :type 'heart :size 32 :color "#E74C3C"))
(define coin-icon (icon :type 'coin :size 32 :color "#F1C40F"))
(define shield-icon (icon :type 'shield :size 32 :color "#3498DB"))
(add (translate-object heart-icon 90 110))
(add (translate-object coin-icon 90 160))
(add (translate-object shield-icon 90 210))

; 数值标签
(add (text 310 115 "72 / 100" :fill "#FFFFFF" :font-size 14))
(add (text 120 165 "x 42" :fill "#FFFFFF" :font-size 14))
(add (text 120 215 "DEF" :fill "#FFFFFF" :font-size 14))

(render "06_ui_components.png")
