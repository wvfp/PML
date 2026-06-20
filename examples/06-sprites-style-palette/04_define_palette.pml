; 示例 4：自定义调色板
; 用 define-palette 注册，再把调色板传给 character 组件使用。
; palette-ref 在 active palette 期间可直接读取颜色（character 组件会自动激活传入的 palette）。

(set-backend! "skia")
(canvas 500 300 :bg "#F8F9FA")

(define-palette "forest-guardian"
  '(("skin" "#F5D0A9")
    ("hair" "#2E7D32")
    ("eyes" "#FFEB3B")
    ("outfit-primary" "#1B5E20")
    ("outfit-secondary" "#4CAF50")
    ("outline" "#1A1A1A")))

; 把自定义调色板应用到角色
(define guardian
  (character :palette "forest-guardian"
             :expression 'surprised
             :style 'cel))

(add (translate-object guardian 250 150))

; 用调色板中的颜色绘制装饰（这里直接引用字符串以作演示）
(add (circle 420 80 30 :fill "#FFEB3B" :stroke "#1A1A1A" :stroke-width 2))
(add (rect 60 210 80 40 :fill "#4CAF50" :stroke "#1A1A1A" :stroke-width 2))

(add (text 250 30 "forest-guardian palette" :fill "#2C3E50" :font-size 18))

(render "04_define_palette.png")
