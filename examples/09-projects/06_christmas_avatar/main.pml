; main.pml - 圣诞头像入口文件
; 组装：hat -> hair -> face -> clothes -> hand 从上到下

(import "palette.pml" as p)
(import "hat.pml" as hat)
(import "hair.pml" as hair)
(import "face.pml" as face)
(import "clothes.pml" as clothes)
(import "hand.pml" as hand)

(set-backend! "skia")
(canvas 1000 1000 :bg p/c-bg)

; 从后到前绘制顺序：衣服（最底层）-> 头发（后）-> 脸 -> 头发（刘海前）-> 帽子 -> 手（最前面）
; 注意：hair-main 和 hat-main 已经包含自身内部层次，这里按整体顺序
(add (clothes/clothes-main))
(add (hair/hair-main))
(add (face/face-main))
(add (hat/hat-main))
(add (hand/hand-main))

(render "06_christmas_avatar.png")
