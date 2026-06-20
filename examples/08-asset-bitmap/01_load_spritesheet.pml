; 示例 1：加载 Sprite Sheet
; 用 load-spritesheet 读取 Aseprite/TexturePacker JSON 图集，并排列成 composition。

(set-backend! "skia")

(define comp (make-composition "spritesheet" 320 128 :bg "#1A1A2E"))

; load-spritesheet 返回裁剪后的 image 对象列表
(define frames (load-spritesheet "../assets/walk_sheet.png" "../assets/walk_sheet.json"))

(println (string-append "Loaded " (number->string (length frames)) " frames"))

; 直接摆放三个帧
(define frame-0 (bitmap-layer "../assets/walk_sheet.png"
                              :name "walk-0"
                              :x 32 :y 32
                              :width 64 :height 64
                              :crop '(0 0 64 64)))

(define frame-1 (bitmap-layer "../assets/walk_sheet.png"
                              :name "walk-1"
                              :x 192 :y 32
                              :width 64 :height 64
                              :crop '(64 0 64 64)))

; 或者用返回的 image 对象构造 layer
(define frame-2 (make-layer "from-list"
                            (list-ref frames 0)
                            :offset '(112 32)
                            :transform (scale 0.75 0.75)))

(set! comp (composition-add comp frame-0 frame-1 frame-2))
(composition-render comp "01_load_spritesheet.png")
