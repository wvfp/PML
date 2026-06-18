;; PML 火柴人角色设计 — Stick Figure Character
;;
;; 运行: python3 -m pml.repl examples/stickman.pml -o examples_output
;;
;; 展示: 多种姿态（站立、跑动、跳跃、挥手）+ 手绘风格

(println "=== PML 火柴人角色设计 ===")

;; ======================================================================
;; 辅助函数 — 火柴人构建块 (head + body + arms + legs)
;; ======================================================================

;; 画火柴人头部 + 躯干（所有姿态共用）
(define (draw-torso cx body-top)
  (define head-r 20)
  (define neck-y (+ body-top head-r 2))
  (define torso-end (+ neck-y 60))
  (define stroke-color "#2c2c2c")
  (define stroke-w 3)
  
  (add (circle cx body-top head-r :fill "none" :stroke stroke-color :stroke-width stroke-w))
  (add (line cx neck-y cx torso-end :stroke stroke-color :stroke-width stroke-w))
  
  ;; 返回 torso-end 和 arm-shoulder-y 供后续使用
  (list torso-end (+ neck-y 8))
)

;; 站立姿态 — 四肢
(define (draw-standing cx torso-end arm-y)
  (define stroke-color "#2c2c2c")
  (define stroke-w 3)
  (add (line cx arm-y (- cx 30) (- torso-end 10) :stroke stroke-color :stroke-width stroke-w))
  (add (line cx arm-y (+ cx 30) (- torso-end 10) :stroke stroke-color :stroke-width stroke-w))
  (add (line cx torso-end (- cx 22) (+ torso-end 60) :stroke stroke-color :stroke-width stroke-w))
  (add (line cx torso-end (+ cx 22) (+ torso-end 60) :stroke stroke-color :stroke-width stroke-w))
)

;; 跑步姿态 — 四肢
(define (draw-running cx body-top torso-end arm-y)
  (define stroke-color "#2c2c2c")
  (define stroke-w 3)
  (add (line cx arm-y (- cx 40) (- torso-end 20) :stroke stroke-color :stroke-width stroke-w))
  (add (line cx arm-y (+ cx 35) (- torso-end 5) :stroke stroke-color :stroke-width stroke-w))
  (add (line cx torso-end (- cx 35) (+ torso-end 55) :stroke stroke-color :stroke-width stroke-w))
  (add (line cx torso-end (+ cx 30) (+ torso-end 45) :stroke stroke-color :stroke-width stroke-w))
)

;; 跳跃姿态 — 四肢
(define (draw-jumping cx body-top torso-end arm-y)
  (define stroke-color "#2c2c2c")
  (define stroke-w 3)
  (add (line cx arm-y (- cx 20) (- body-top 30) :stroke stroke-color :stroke-width stroke-w))
  (add (line cx arm-y (+ cx 20) (- body-top 30) :stroke stroke-color :stroke-width stroke-w))
  (add (line cx torso-end (- cx 30) torso-end :stroke stroke-color :stroke-width stroke-w))
  (add (line cx torso-end (+ cx 30) torso-end :stroke stroke-color :stroke-width stroke-w))
  (add (line (- cx 30) torso-end (- cx 25) (+ torso-end 35) :stroke stroke-color :stroke-width stroke-w))
  (add (line (+ cx 30) torso-end (+ cx 25) (+ torso-end 35) :stroke stroke-color :stroke-width stroke-w))
)

;; 挥手姿态 — 四肢
(define (draw-waving cx torso-end arm-y)
  (define stroke-color "#2c2c2c")
  (define stroke-w 3)
  (add (line cx arm-y (+ cx 40) (- (+ arm-y (- torso-end arm-y)) 100) :stroke stroke-color :stroke-width stroke-w))
  (add (line cx arm-y (- cx 30) (- torso-end 10) :stroke stroke-color :stroke-width stroke-w))
  (add (line cx torso-end (- cx 22) (+ torso-end 60) :stroke stroke-color :stroke-width stroke-w))
  (add (line cx torso-end (+ cx 22) (+ torso-end 60) :stroke stroke-color :stroke-width stroke-w))
)

;; 主函数 — 按姿态分发
(define (draw-stickman cx body-top pose)
  (define body-info (draw-torso cx body-top))
  (define torso-end (car body-info))
  (define arm-y (car (cdr body-info)))
  
  (if (= pose 'standing) (draw-standing cx torso-end arm-y)
    (if (= pose 'running) (draw-running cx body-top torso-end arm-y)
      (if (= pose 'jumping) (draw-jumping cx body-top torso-end arm-y)
        (if (= pose 'waving) (draw-waving cx torso-end arm-y)
          (draw-standing cx torso-end arm-y)))))
)


;; ======================================================================
;; 1. 火柴人 — 站立姿态（干净风格）
;; ======================================================================
(println "\n--- 1. 站立姿态 ---")

(canvas 200 280 :bg "#f8f8f8")
(draw-stickman 100 55 'standing)
(render "examples_output/stickman_standing.png")
(println "  stickman_standing.png rendered!")


;; ======================================================================
;; 2. 火柴人 — 跑步姿态
;; ======================================================================
(println "\n--- 2. 跑步姿态 ---")

(canvas 200 280 :bg "#f8f8f8")
(draw-stickman 100 55 'running)
(render "examples_output/stickman_running.png")
(println "  stickman_running.png rendered!")


;; ======================================================================
;; 3. 火柴人 — 跳跃姿态
;; ======================================================================
(println "\n--- 3. 跳跃姿态 ---")

(canvas 200 280 :bg "#f8f8f8")
(draw-stickman 100 55 'jumping)
(render "examples_output/stickman_jumping.png")
(println "  stickman_jumping.png rendered!")


;; ======================================================================
;; 4. 火柴人 — 挥手姿态
;; ======================================================================
(println "\n--- 4. 挥手姿态 ---")

(canvas 200 280 :bg "#f8f8f8")
(draw-stickman 100 55 'waving)
(render "examples_output/stickman_waving.png")
(println "  stickman_waving.png rendered!")


;; ======================================================================
;; 5. 手绘风格火柴人（铅笔 + 纸纹理）
;; ======================================================================
(println "\n--- 5. 手绘风格 ---")

(canvas 200 280 :bg "#f5f0e8")

(define head-r 20)
(define neck-y (+ 55 head-r 2))
(define torso-end (+ neck-y 60))
(define arm-y (+ neck-y 8))

;; 头（sketchify 手绘圆圈）
(add (sketchify (circle 100 55 head-r :fill "none" :stroke "#2c2c2c" :stroke-width 2.5) :roughness 0.25))

;; 躯干（pencil 线 — 自然抖动）
(add (pencil 100 neck-y 100 torso-end :stroke "#2c2c2c" :stroke-width 2.5 :roughness 0.3 :variance 0.4))

;; 双臂（pencil）
(add (pencil 100 arm-y (- 100 30) (- torso-end 10)
             :stroke "#2c2c2c" :stroke-width 2.5 :roughness 0.3 :variance 0.4))
(add (pencil 100 arm-y (+ 100 30) (- torso-end 10)
             :stroke "#2c2c2c" :stroke-width 2.5 :roughness 0.3 :variance 0.4))

;; 双腿（pencil）
(add (pencil 100 torso-end (- 100 22) (+ torso-end 60)
             :stroke "#2c2c2c" :stroke-width 2.5 :roughness 0.3 :variance 0.4))
(add (pencil 100 torso-end (+ 100 22) (+ torso-end 60)
             :stroke "#2c2c2c" :stroke-width 2.5 :roughness 0.3 :variance 0.4))

(render "examples_output/stickman_handdrawn.png")
(println "  stickman_handdrawn.png rendered!")


;; ======================================================================
;; 6. 四格组图 — 火柴人动画序列
;; ======================================================================
(println "\n--- 6. 四格动画序列 ---")

(canvas 800 280 :bg "#f8f8f8")

;; 画分隔线
(add (line 200 0 200 280 :stroke "#ddd" :stroke-width 1))
(add (line 400 0 400 280 :stroke "#ddd" :stroke-width 1))
(add (line 600 0 600 280 :stroke "#ddd" :stroke-width 1))

;; 4 个姿态分别画在不同区域
(draw-stickman 100  55 'standing)
(draw-stickman 300  55 'running)
(draw-stickman 500  55 'jumping)
(draw-stickman 700  55 'waving)

;; 标注文字
(add (text "站立" 80 260 :fill "#888" :size 14))
(add (text "跑步" 285 260 :fill "#888" :size 14))
(add (text "跳跃" 485 260 :fill "#888" :size 14))
(add (text "挥手" 690 260 :fill "#888" :size 14))

(render "examples_output/stickman_sequence.png")
(println "  stickman_sequence.png rendered!")


;; ======================================================================
;; 7. 火柴人团队 — 多角色
;; ======================================================================
(println "\n--- 7. 火柴人团队 ---")

(canvas 400 280 :bg "#f0f4f8")

;; 三个火柴人站一排
(draw-stickman 80  55 'waving)
(draw-stickman 200 55 'standing)
(draw-stickman 320 55 'jumping)

(render "examples_output/stickman_team.png")
(println "  stickman_team.png rendered!")


(println "\n=== 全部渲染完成！===")