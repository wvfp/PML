; 03_doubao_logo - 复刻参考图 tp.png 中的彩色 logo
; 整体结构：黑色背景 + 白色圆角方块 + 镜头 + 彩轮齿轮 + 三个装饰元素 + 加号按钮
; 各部分拆分到独立模块，便于维护。

(set-backend! "skia")
(canvas 800 800 :bg "#000000")

(import "background.pml" as bg)
(import "lens.pml" as lens)
(import "arc.pml" as arc)
(import "decorations.pml" as deco)
(import "plus_button.pml" as btn)

; 背景白卡片
(add (bg/white-card))

; 彩轮齿轮（在镜头后面）
(add (arc/color-wheel))

; 装饰元素（粉旗、绿叶、蓝水滴）
(add (deco/cluster))

; 镜头（盖在彩轮中间）
(add (lens/camera-lens))

; 加号按钮
(add (btn/plus-button))

(render "03_doubao_logo.png")
