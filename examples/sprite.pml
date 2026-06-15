; PML Phase 2+3 示例 — 图形图元与角色 sprite 生成
; 输出：基础图元测试图 + 完整角色 sprite

; ============================
; 1. 基础图元测试
; ============================
(println "=== 1. Basic Shapes ===")

(canvas 300 200 :bg "#f5f5f5")
(add (rect 20 20 80 60 :fill "#3498db" :stroke "#2c3e50" :stroke-width 2))
(add (circle 180 60 35 :fill "#e74c3c" :stroke "#c0392b" :stroke-width 2))
(add (ellipse 260 100 25 40 :fill "#2ecc71" :stroke "#27ae60" :stroke-width 2))
(add (line 20 150 280 150 :stroke "#9b59b6" :stroke-width 3))
(add (polygon (list (list 50 160) (list 80 190) (list 20 190)) :fill "#f39c12" :stroke "#e67e22"))
(render "examples_output/shapes.png")
(println "  shapes.png rendered!")

; ============================
; 2. 变换矩阵
; ============================
(println "\n=== 2. Transforms ===")

(define t1 (translate 100 50))
(define t2 (scale 2 2))
(define t3 (compose t1 t2))
(println "  translate:" t1)
(println "  scale:" t2)
(println "  composed:" t3)
(println "  apply (5,0):" (matrix-apply t3 5 0))
(println "  identity?" (matrix? (identity-matrix)))

; ============================
; 3. 风格系统
; ============================
(println "\n=== 3. Style System ===")

(define-style "neon"
  :outline-width 3.0
  :outline-color "#00ff88"
  :shading "cel"
  :highlight #t)

(println "  cel style:" (use-style 'cel))
(println "  neon style:" (use-style "neon"))

; ============================
; 4. 独立组件测试
; ============================
(println "\n=== 4. Components ===")

(define my-body (body :skin "#fce4c8" :build "slim" :proportions "anime"))
(println "  body:" my-body)

(define my-head (head :shape "oval" :skin "#fce4c8" :ears "normal"))
(println "  head:" my-head)

(define my-eyes (anime-eyes :style "shoujo" :color "#4a90d9" :size 1.0))
(println "  eyes:" my-eyes)

(define my-mouth (mouth :style "smile" :size 1.0))
(println "  mouth:" my-mouth)

(define my-hair (hair :style "long" :color "#8B4513" :bangs #t))
(println "  hair:" my-hair)

; ============================
; 5. 角色 sprite 生成
; ============================
(println "\n=== 5. Character Sprite ===")

(sprite-canvas 128 256 :bg "transparent" :anchor 'center-bottom)

(define hero
  (character
    :expression "happy"
    :style 'cel))

(add hero)
(render "examples_output/hero.png")
(println "  hero.png rendered!")

; ============================
; 6. 不同表情对比
; ============================
(println "\n=== 6. Expression Variants ===")

(sprite-canvas 128 256 :bg "transparent")
(add (character :expression "angry" :style 'cel))
(render "examples_output/hero_angry.png")
(println "  hero_angry.png rendered!")

(sprite-canvas 128 256 :bg "transparent")
(add (character :expression "surprised" :style 'cel))
(render "examples_output/hero_surprised.png")
(println "  hero_surprised.png rendered!")

(println "\n=== All Phase 2+3 examples completed successfully ===")
