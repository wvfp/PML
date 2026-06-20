; 示例 4：组合与变换
; 展示 group、with-transform、translate-object、rotate-object、scale-object。
; 绘制一朵由多个旋转花瓣组成的花朵。

(set-backend! "skia")
(canvas 400 400 :bg "#F8F9FA")

; 单个花瓣原型，中心在原点上方
(define petal
  (ellipse 0 -60 20 50
           :fill "#FF922B"
           :stroke "#D9480F"
           :stroke-width 2))

; 用 group 把旋转后的花瓣组合在一起
(define flower
  (group
    (rotate-object petal 0)
    (rotate-object petal 45)
    (rotate-object petal 90)
    (rotate-object petal 135)
    (rotate-object petal 180)
    (rotate-object petal 225)
    (rotate-object petal 270)
    (rotate-object petal 315)))

; 将整朵花平移到画布中心
(add (translate-object flower 200 200))

; 花心
(add (circle 200 200 30
             :fill "#FCC419"
             :stroke "#E67700"
             :stroke-width 2))

; 右下角再放一个经过缩放和 with-transform 变换的小方块
(define square
  (rect -15 -15 30 30
        :fill "#339AF0"
        :stroke "#1864AB"
        :stroke-width 2))

; compose 一次只能组合两个变换，嵌套即可得到 translate → rotate → scale 的复合效果
(add (with-transform square
                     (compose (translate 320 320)
                              (compose (rotate 30)
                                       (scale 1.5 1.5)))))

(render "04_groups_and_transforms.png")
