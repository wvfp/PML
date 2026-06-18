;; PML 火柴人角色设计 v2 — 优化版 + 动画
;;
;; 运行: source .venv/bin/activate && python3 -m pml.repl examples/stickman_v2.pml -o examples_output
;;
;; 改进:
;;   1) 比例优化，关节圆点
;;   2) 地面投影
;;   3) 行走动画循环
;;   4) 挥手动画
;;   5) 跳跃动画

(println "=== PML 火柴人 v2 ===")

;; ======================================================================
;; 常量
;; ======================================================================
(define CX 100)               ;; 身体中线
(define HEAD-R 16)             ;; 头半径
(define HEAD-CY 48)            ;; 头中心 Y
(define NECK-Y 64)             ;; 脖子 Y
(define SHOULDER-Y 68)         ;; 肩膀 Y
(define HIP-Y 125)             ;; 髋部 Y
(define TORSO-LEN (- HIP-Y NECK-Y))
(define GROUND-Y 195)          ;; 地面 Y

(define STROKE "#2a2a2a")
(define SW 3.0)                ;; 线条宽度
(define JOINT-R 3.5)           ;; 关节点半径
(define FOOT-R 4)              ;; 脚部半径

;; ======================================================================
;; 单帧姿态函数（用于静态渲染）
;; ======================================================================

(define (draw-head cx cy)
  (add (circle cx cy HEAD-R :fill "none" :stroke STROKE :stroke-width SW))
  ;; 眼睛 — 两个小点
  (add (circle (- cx 5) (- cy 2) 1.5 :fill STROKE))
  (add (circle (+ cx 5) (- cy 2) 1.5 :fill STROKE))
  ;; 微笑弧线
  (add (line (- cx 5) (+ cy 3) cx (+ cy 6) :stroke STROKE :stroke-width 1.5))
  (add (line cx (+ cy 6) (+ cx 5) (+ cy 3) :stroke STROKE :stroke-width 1.5))
)

(define (draw-joint x y)
  (add (circle x y JOINT-R :fill STROKE))
)

(define (draw-foot x y)
  (add (circle x y FOOT-R :fill STROKE))
)

(define (draw-torso)
  (add (line CX NECK-Y CX HIP-Y :stroke STROKE :stroke-width SW))
  (draw-joint CX SHOULDER-Y)
  (draw-joint CX HIP-Y)
)

(define (draw-ground)
  ;; 地面线 + 阴影
  (add (line 30 GROUND-Y 170 GROUND-Y :stroke "#bbb" :stroke-width 1))
  (add (ellipse CX (+ GROUND-Y 6) 50 4 :fill "rgba(0,0,0,0.08)"))
)

;; 站立 — 四肢
(define (draw-standing-limbs)
  (add (line CX SHOULDER-Y (- CX 28) (- HIP-Y 15) :stroke STROKE :stroke-width SW))
  (add (line CX SHOULDER-Y (+ CX 28) (- HIP-Y 15) :stroke STROKE :stroke-width SW))
  (add (line CX HIP-Y (- CX 22) GROUND-Y :stroke STROKE :stroke-width SW))
  (add (line CX HIP-Y (+ CX 22) GROUND-Y :stroke STROKE :stroke-width SW))
  ;; 手脚关节点
  (draw-joint (- CX 28) (- HIP-Y 15))
  (draw-joint (+ CX 28) (- HIP-Y 15))
  (draw-foot (- CX 22) GROUND-Y)
  (draw-foot (+ CX 22) GROUND-Y)
)

;; 跑步 — 四肢
(define (draw-running-limbs)
  (add (line CX SHOULDER-Y (- CX 38) (- HIP-Y 25) :stroke STROKE :stroke-width SW))
  (add (line CX SHOULDER-Y (+ CX 30) (- HIP-Y 10) :stroke STROKE :stroke-width SW))
  (add (line CX HIP-Y (- CX 32) (+ GROUND-Y 2) :stroke STROKE :stroke-width SW))
  (add (line CX HIP-Y (+ CX 28) (- GROUND-Y 5) :stroke STROKE :stroke-width SW))
  (draw-joint (- CX 38) (- HIP-Y 25))
  (draw-joint (+ CX 30) (- HIP-Y 10))
  (draw-foot (- CX 32) (+ GROUND-Y 2))
  (draw-foot (+ CX 28) (- GROUND-Y 5))
)

;; 跳跃 — 四肢
(define (draw-jumping-limbs)
  (add (line CX SHOULDER-Y (- CX 18) (- HEAD-CY HEAD-R 8) :stroke STROKE :stroke-width SW))
  (add (line CX SHOULDER-Y (+ CX 18) (- HEAD-CY HEAD-R 8) :stroke STROKE :stroke-width SW))
  (add (line CX HIP-Y (- CX 28) HIP-Y :stroke STROKE :stroke-width SW))
  (add (line CX HIP-Y (+ CX 28) HIP-Y :stroke STROKE :stroke-width SW))
  (add (line (- CX 28) HIP-Y (- CX 22) (- GROUND-Y 10) :stroke STROKE :stroke-width SW))
  (add (line (+ CX 28) HIP-Y (+ CX 22) (- GROUND-Y 10) :stroke STROKE :stroke-width SW))
  (draw-joint (- CX 18) (- HEAD-CY HEAD-R 8))
  (draw-joint (+ CX 18) (- HEAD-CY HEAD-R 8))
  (draw-joint (- CX 28) HIP-Y)
  (draw-joint (+ CX 28) HIP-Y)
  (draw-foot (- CX 22) (- GROUND-Y 10))
  (draw-foot (+ CX 22) (- GROUND-Y 10))
)

;; 挥手 — 四肢
(define (draw-waving-limbs)
  (add (line CX SHOULDER-Y (+ CX 35) (- HEAD-CY 30) :stroke STROKE :stroke-width SW))
  (add (line CX SHOULDER-Y (- CX 28) (- HIP-Y 15) :stroke STROKE :stroke-width SW))
  (add (line CX HIP-Y (- CX 22) GROUND-Y :stroke STROKE :stroke-width SW))
  (add (line CX HIP-Y (+ CX 22) GROUND-Y :stroke STROKE :stroke-width SW))
  (draw-joint (+ CX 35) (- HEAD-CY 30))
  (draw-joint (- CX 28) (- HIP-Y 15))
  (draw-foot (- CX 22) GROUND-Y)
  (draw-foot (+ CX 22) GROUND-Y)
)

;; 主分发函数
(define (draw-scene pose)
  (draw-ground)
  (draw-head CX HEAD-CY)
  (draw-torso)
  (if (= pose 'standing) (draw-standing-limbs)
    (if (= pose 'running) (draw-running-limbs)
      (if (= pose 'jumping) (draw-jumping-limbs)
        (if (= pose 'waving) (draw-waving-limbs)
          (draw-standing-limbs)))))
)


;; ======================================================================
;; 1-4. 单帧渲染（各姿态）
;; ======================================================================
(println "\n--- 单帧姿态 ---")

(canvas 200 280 :bg "#fafafa")
(draw-scene 'standing)
(render "examples_output/stickman_v2_standing.png")
(println "  stickman_v2_standing.png")

(canvas 200 280 :bg "#fafafa")
(draw-scene 'running)
(render "examples_output/stickman_v2_running.png")
(println "  stickman_v2_running.png")

(canvas 200 280 :bg "#fafafa")
(draw-scene 'jumping)
(render "examples_output/stickman_v2_jumping.png")
(println "  stickman_v2_jumping.png")

(canvas 200 280 :bg "#fafafa")
(draw-scene 'waving)
(render "examples_output/stickman_v2_waving.png")
(println "  stickman_v2_waving.png")


;; ======================================================================
;; 5. 行走动画 GIF
;; ======================================================================
(println "\n--- 5. 行走动画 ---")

(canvas 200 280 :bg "#fafafa")

;; 地面（静态）
(add (line 30 GROUND-Y 170 GROUND-Y :stroke "#bbb" :stroke-width 1))
(add (ellipse CX (+ GROUND-Y 6) 45 4 :fill "rgba(0,0,0,0.06)"))

;; 头部（静态）
(add (circle CX HEAD-CY HEAD-R :fill "none" :stroke STROKE :stroke-width SW))
(add (circle (- CX 5) (- HEAD-CY 2) 1.5 :fill STROKE))
(add (circle (+ CX 5) (- HEAD-CY 2) 1.5 :fill STROKE))
(add (line (- CX 5) (+ HEAD-CY 3) CX (+ HEAD-CY 6) :stroke STROKE :stroke-width 1.5))
(add (line CX (+ HEAD-CY 6) (+ CX 5) (+ HEAD-CY 3) :stroke STROKE :stroke-width 1.5))

;; 躯干（静态）
(add (line CX NECK-Y CX HIP-Y :stroke STROKE :stroke-width SW))
;; 肩膀和髋部关节点
(add (circle CX SHOULDER-Y JOINT-R :fill STROKE))
(add (circle CX HIP-Y JOINT-R :fill STROKE))

;; === 可动画的四肢 — 存储到变量 ===

;; 左臂: 肩膀(CX, SHOULDER-Y) → 手
(define left-arm (line CX SHOULDER-Y (- CX 35) (- HIP-Y 20) :stroke STROKE :stroke-width SW))
;; 右臂: 肩膀(CX, SHOULDER-Y) → 手
(define right-arm (line CX SHOULDER-Y (+ CX 30) (- HIP-Y 10) :stroke STROKE :stroke-width SW))
;; 左腿: 髋部(CX, HIP-Y) → 脚
(define left-leg (line CX HIP-Y (- CX 28) GROUND-Y :stroke STROKE :stroke-width SW))
;; 右腿: 髋部(CX, HIP-Y) → 脚
(define right-leg (line CX HIP-Y (+ CX 28) GROUND-Y :stroke STROKE :stroke-width SW))

;; 手肘/膝盖关节点
(define left-elbow (circle (- CX 35) (- HIP-Y 20) JOINT-R :fill STROKE))
(define right-elbow (circle (+ CX 30) (- HIP-Y 10) JOINT-R :fill STROKE))
(define left-knee (circle (- CX 28) GROUND-Y FOOT-R :fill STROKE))
(define right-knee (circle (+ CX 28) GROUND-Y FOOT-R :fill STROKE))

(add left-arm)
(add right-arm)
(add left-leg)
(add right-leg)
(add left-elbow)
(add right-elbow)
(add left-knee)
(add right-knee)

;; === 行走动画: 四肢端点摆动 ===
;; 周期 1.0s, 使用 repeat 'infinite

;; 左臂: 前摆 (cx-35, hip-20) ↔ 后摆 (cx+28, hip-8)
(animate left-arm 'x2 (- CX 35) (+ CX 28) 0.5 :ease 'sine-in-out :repeat 'infinite)
(animate left-arm 'x2 (+ CX 28) (- CX 35) 0.5 :ease 'sine-in-out :delay 0.5 :repeat 'infinite)
(animate left-arm 'y2 (- HIP-Y 20) (- HIP-Y 8) 0.5 :ease 'sine-in-out :repeat 'infinite)
(animate left-arm 'y2 (- HIP-Y 8) (- HIP-Y 20) 0.5 :ease 'sine-in-out :delay 0.5 :repeat 'infinite)

;; 右臂: 反相位 (后摆 ↔ 前摆)
(animate right-arm 'x2 (+ CX 30) (- CX 32) 0.5 :ease 'sine-in-out :repeat 'infinite)
(animate right-arm 'x2 (- CX 32) (+ CX 30) 0.5 :ease 'sine-in-out :delay 0.5 :repeat 'infinite)
(animate right-arm 'y2 (- HIP-Y 10) (- HIP-Y 22) 0.5 :ease 'sine-in-out :repeat 'infinite)
(animate right-arm 'y2 (- HIP-Y 22) (- HIP-Y 10) 0.5 :ease 'sine-in-out :delay 0.5 :repeat 'infinite)

;; 左腿: 前跨 (cx-28, ground) ↔ 后蹬 (cx+30, ground-5)
(animate left-leg 'x2 (- CX 28) (+ CX 30) 0.5 :ease 'sine-in-out :repeat 'infinite)
(animate left-leg 'x2 (+ CX 30) (- CX 28) 0.5 :ease 'sine-in-out :delay 0.5 :repeat 'infinite)
(animate left-leg 'y2 GROUND-Y (- GROUND-Y 5) 0.5 :ease 'sine-in-out :repeat 'infinite)
(animate left-leg 'y2 (- GROUND-Y 5) GROUND-Y 0.5 :ease 'sine-in-out :delay 0.5 :repeat 'infinite)

;; 右腿: 反相位
(animate right-leg 'x2 (+ CX 28) (- CX 30) 0.5 :ease 'sine-in-out :repeat 'infinite)
(animate right-leg 'x2 (- CX 30) (+ CX 28) 0.5 :ease 'sine-in-out :delay 0.5 :repeat 'infinite)
(animate right-leg 'y2 GROUND-Y (- GROUND-Y 3) 0.5 :ease 'sine-in-out :repeat 'infinite)
(animate right-leg 'y2 (- GROUND-Y 3) GROUND-Y 0.5 :ease 'sine-in-out :delay 0.5 :repeat 'infinite)

;; 关节点跟随四肢端点
(animate left-elbow 'cx (- CX 35) (+ CX 28) 0.5 :ease 'sine-in-out :repeat 'infinite)
(animate left-elbow 'cx (+ CX 28) (- CX 35) 0.5 :ease 'sine-in-out :delay 0.5 :repeat 'infinite)
(animate left-elbow 'cy (- HIP-Y 20) (- HIP-Y 8) 0.5 :ease 'sine-in-out :repeat 'infinite)
(animate left-elbow 'cy (- HIP-Y 8) (- HIP-Y 20) 0.5 :ease 'sine-in-out :delay 0.5 :repeat 'infinite)

(animate right-elbow 'cx (+ CX 30) (- CX 32) 0.5 :ease 'sine-in-out :repeat 'infinite)
(animate right-elbow 'cx (- CX 32) (+ CX 30) 0.5 :ease 'sine-in-out :delay 0.5 :repeat 'infinite)
(animate right-elbow 'cy (- HIP-Y 10) (- HIP-Y 22) 0.5 :ease 'sine-in-out :repeat 'infinite)
(animate right-elbow 'cy (- HIP-Y 22) (- HIP-Y 10) 0.5 :ease 'sine-in-out :delay 0.5 :repeat 'infinite)

(animate left-knee 'cx (- CX 28) (+ CX 30) 0.5 :ease 'sine-in-out :repeat 'infinite)
(animate left-knee 'cx (+ CX 30) (- CX 28) 0.5 :ease 'sine-in-out :delay 0.5 :repeat 'infinite)
(animate left-knee 'cy GROUND-Y (- GROUND-Y 5) 0.5 :ease 'sine-in-out :repeat 'infinite)
(animate left-knee 'cy (- GROUND-Y 5) GROUND-Y 0.5 :ease 'sine-in-out :delay 0.5 :repeat 'infinite)

(animate right-knee 'cx (+ CX 28) (- CX 30) 0.5 :ease 'sine-in-out :repeat 'infinite)
(animate right-knee 'cx (- CX 30) (+ CX 28) 0.5 :ease 'sine-in-out :delay 0.5 :repeat 'infinite)
(animate right-knee 'cy GROUND-Y (- GROUND-Y 3) 0.5 :ease 'sine-in-out :repeat 'infinite)
(animate right-knee 'cy (- GROUND-Y 3) GROUND-Y 0.5 :ease 'sine-in-out :delay 0.5 :repeat 'infinite)

(play)
(render "examples_output/stickman_v2_walk.gif" :fps 20)
(println "  stickman_v2_walk.gif")


;; ======================================================================
;; 6. 挥手动画 GIF
;; ======================================================================
(println "\n--- 6. 挥手动画 ---")

(canvas 200 280 :bg "#fafafa")

;; 地面（静态）
(add (line 30 GROUND-Y 170 GROUND-Y :stroke "#bbb" :stroke-width 1))
(add (ellipse CX (+ GROUND-Y 6) 45 4 :fill "rgba(0,0,0,0.06)"))

;; 头部（静态）
(add (circle CX HEAD-CY HEAD-R :fill "none" :stroke STROKE :stroke-width SW))
(add (circle (- CX 5) (- HEAD-CY 2) 1.5 :fill STROKE))
(add (circle (+ CX 5) (- HEAD-CY 2) 1.5 :fill STROKE))
(add (line (- CX 5) (+ HEAD-CY 3) CX (+ HEAD-CY 6) :stroke STROKE :stroke-width 1.5))
(add (line CX (+ HEAD-CY 6) (+ CX 5) (+ HEAD-CY 3) :stroke STROKE :stroke-width 1.5))

;; 躯干（静态）
(add (line CX NECK-Y CX HIP-Y :stroke STROKE :stroke-width SW))
(add (circle CX SHOULDER-Y JOINT-R :fill STROKE))
(add (circle CX HIP-Y JOINT-R :fill STROKE))

;; 左臂（静态下垂）
(add (line CX SHOULDER-Y (- CX 28) (- HIP-Y 15) :stroke STROKE :stroke-width SW))
(add (circle (- CX 28) (- HIP-Y 15) JOINT-R :fill STROKE))

;; 双腿（静态站立）
(add (line CX HIP-Y (- CX 22) GROUND-Y :stroke STROKE :stroke-width SW))
(add (line CX HIP-Y (+ CX 22) GROUND-Y :stroke STROKE :stroke-width SW))
(add (circle (- CX 22) GROUND-Y FOOT-R :fill STROKE))
(add (circle (+ CX 22) GROUND-Y FOOT-R :fill STROKE))

;; 右臂 — 挥手（可动画）
(define waving-arm (line CX SHOULDER-Y (+ CX 35) (- HEAD-CY 25) :stroke STROKE :stroke-width SW))
(define waving-elbow (circle (+ CX 35) (- HEAD-CY 25) JOINT-R :fill STROKE))
(add waving-arm)
(add waving-elbow)

;; 挥手动画：手臂上下摆动
(animate waving-arm 'x2 (+ CX 35) (+ CX 38) 0.3 :ease 'sine-in-out :repeat 'infinite)
(animate waving-arm 'x2 (+ CX 38) (+ CX 35) 0.3 :ease 'sine-in-out :delay 0.3 :repeat 'infinite)
(animate waving-arm 'y2 (- HEAD-CY 25) (- HEAD-CY 40) 0.3 :ease 'sine-in-out :repeat 'infinite)
(animate waving-arm 'y2 (- HEAD-CY 40) (- HEAD-CY 25) 0.3 :ease 'sine-in-out :delay 0.3 :repeat 'infinite)

(animate waving-elbow 'cx (+ CX 35) (+ CX 38) 0.3 :ease 'sine-in-out :repeat 'infinite)
(animate waving-elbow 'cx (+ CX 38) (+ CX 35) 0.3 :ease 'sine-in-out :delay 0.3 :repeat 'infinite)
(animate waving-elbow 'cy (- HEAD-CY 25) (- HEAD-CY 40) 0.3 :ease 'sine-in-out :repeat 'infinite)
(animate waving-elbow 'cy (- HEAD-CY 40) (- HEAD-CY 25) 0.3 :ease 'sine-in-out :delay 0.3 :repeat 'infinite)

(play)
(render "examples_output/stickman_v2_wave.gif" :fps 15)
(println "  stickman_v2_wave.gif")


;; ======================================================================
;; 7. 跳跃动画 GIF
;; ======================================================================
(println "\n--- 7. 跳跃动画 ---")

(canvas 200 280 :bg "#fafafa")

;; 地面（静态）
(add (line 30 GROUND-Y 170 GROUND-Y :stroke "#bbb" :stroke-width 1))

;; === 跳跃状态变量 ===
(define jump-head-cy HEAD-CY)
(define jump-neck-y NECK-Y)
(define jump-hip-y HIP-Y)

;; 头部（需要上下动）
(define jump-head (circle CX HEAD-CY HEAD-R :fill "none" :stroke STROKE :stroke-width SW))
(add jump-head)
(add (circle (- CX 5) (- HEAD-CY 2) 1.5 :fill STROKE))
(add (circle (+ CX 5) (- HEAD-CY 2) 1.5 :fill STROKE))
(add (line (- CX 5) (+ HEAD-CY 3) CX (+ HEAD-CY 6) :stroke STROKE :stroke-width 1.5))
(add (line CX (+ HEAD-CY 6) (+ CX 5) (+ HEAD-CY 3) :stroke STROKE :stroke-width 1.5))

;; 躯干（上下动）
(define jump-torso (line CX NECK-Y CX HIP-Y :stroke STROKE :stroke-width SW))
(define jump-shoulder (circle CX SHOULDER-Y JOINT-R :fill STROKE))
(define jump-hip (circle CX HIP-Y JOINT-R :fill STROKE))
(add jump-torso)
(add jump-shoulder)
(add jump-hip)

;; 双臂上举
(define jump-l-arm (line CX SHOULDER-Y (- CX 18) (- HEAD-CY 30) :stroke STROKE :stroke-width SW))
(define jump-r-arm (line CX SHOULDER-Y (+ CX 18) (- HEAD-CY 30) :stroke STROKE :stroke-width SW))
(define jump-l-elbow (circle (- CX 18) (- HEAD-CY 30) JOINT-R :fill STROKE))
(define jump-r-elbow (circle (+ CX 18) (- HEAD-CY 30) JOINT-R :fill STROKE))
(add jump-l-arm)
(add jump-r-arm)
(add jump-l-elbow)
(add jump-r-elbow)

;; 双腿分开弯曲
(define jump-l-leg1 (line CX HIP-Y (- CX 28) HIP-Y :stroke STROKE :stroke-width SW))
(define jump-r-leg1 (line CX HIP-Y (+ CX 28) HIP-Y :stroke STROKE :stroke-width SW))
(define jump-l-leg2 (line (- CX 28) HIP-Y (- CX 22) (- GROUND-Y 10) :stroke STROKE :stroke-width SW))
(define jump-r-leg2 (line (+ CX 28) HIP-Y (+ CX 22) (- GROUND-Y 10) :stroke STROKE :stroke-width SW))
(define jump-l-knee (circle (- CX 28) HIP-Y JOINT-R :fill STROKE))
(define jump-r-knee (circle (+ CX 28) HIP-Y JOINT-R :fill STROKE))
(define jump-l-foot (circle (- CX 22) (- GROUND-Y 10) FOOT-R :fill STROKE))
(define jump-r-foot (circle (+ CX 22) (- GROUND-Y 10) FOOT-R :fill STROKE))
(add jump-l-leg1)
(add jump-r-leg1)
(add jump-l-leg2)
(add jump-r-leg2)
(add jump-l-knee)
(add jump-r-knee)
(add jump-l-foot)
(add jump-r-foot)

;; 跳跃动画 — 整个角色 Y 轴上下弹跳
(define bounce-offset 20)

(animate jump-head 'cy HEAD-CY (- HEAD-CY bounce-offset) 0.4 :ease 'quad-out :repeat 'infinite)
(animate jump-head 'cy (- HEAD-CY bounce-offset) HEAD-CY 0.4 :ease 'quad-in :delay 0.4 :repeat 'infinite)

(animate jump-torso 'y1 NECK-Y (- NECK-Y bounce-offset) 0.4 :ease 'quad-out :repeat 'infinite)
(animate jump-torso 'y1 (- NECK-Y bounce-offset) NECK-Y 0.4 :ease 'quad-in :delay 0.4 :repeat 'infinite)
(animate jump-torso 'y2 HIP-Y (- HIP-Y bounce-offset) 0.4 :ease 'quad-out :repeat 'infinite)
(animate jump-torso 'y2 (- HIP-Y bounce-offset) HIP-Y 0.4 :ease 'quad-in :delay 0.4 :repeat 'infinite)

(animate jump-shoulder 'cy SHOULDER-Y (- SHOULDER-Y bounce-offset) 0.4 :ease 'quad-out :repeat 'infinite)
(animate jump-shoulder 'cy (- SHOULDER-Y bounce-offset) SHOULDER-Y 0.4 :ease 'quad-in :delay 0.4 :repeat 'infinite)

(animate jump-hip 'cy HIP-Y (- HIP-Y bounce-offset) 0.4 :ease 'quad-out :repeat 'infinite)
(animate jump-hip 'cy (- HIP-Y bounce-offset) HIP-Y 0.4 :ease 'quad-in :delay 0.4 :repeat 'infinite)

;; 手臂和腿也跟着上下
(animate jump-l-arm 'y1 SHOULDER-Y (- SHOULDER-Y bounce-offset) 0.4 :ease 'quad-out :repeat 'infinite)
(animate jump-l-arm 'y1 (- SHOULDER-Y bounce-offset) SHOULDER-Y 0.4 :ease 'quad-in :delay 0.4 :repeat 'infinite)
(animate jump-l-arm 'y2 (- HEAD-CY 30) (- (- HEAD-CY bounce-offset) 30) 0.4 :ease 'quad-out :repeat 'infinite)
(animate jump-l-arm 'y2 (- (- HEAD-CY bounce-offset) 30) (- HEAD-CY 30) 0.4 :ease 'quad-in :delay 0.4 :repeat 'infinite)

(animate jump-r-arm 'y1 SHOULDER-Y (- SHOULDER-Y bounce-offset) 0.4 :ease 'quad-out :repeat 'infinite)
(animate jump-r-arm 'y1 (- SHOULDER-Y bounce-offset) SHOULDER-Y 0.4 :ease 'quad-in :delay 0.4 :repeat 'infinite)
(animate jump-r-arm 'y2 (- HEAD-CY 30) (- (- HEAD-CY bounce-offset) 30) 0.4 :ease 'quad-out :repeat 'infinite)
(animate jump-r-arm 'y2 (- (- HEAD-CY bounce-offset) 30) (- HEAD-CY 30) 0.4 :ease 'quad-in :delay 0.4 :repeat 'infinite)

(animate jump-l-leg1 'y1 HIP-Y (- HIP-Y bounce-offset) 0.4 :ease 'quad-out :repeat 'infinite)
(animate jump-l-leg1 'y1 (- HIP-Y bounce-offset) HIP-Y 0.4 :ease 'quad-in :delay 0.4 :repeat 'infinite)
(animate jump-l-leg1 'y2 HIP-Y (- HIP-Y bounce-offset) 0.4 :ease 'quad-out :repeat 'infinite)
(animate jump-l-leg1 'y2 (- HIP-Y bounce-offset) HIP-Y 0.4 :ease 'quad-in :delay 0.4 :repeat 'infinite)

(animate jump-r-leg1 'y1 HIP-Y (- HIP-Y bounce-offset) 0.4 :ease 'quad-out :repeat 'infinite)
(animate jump-r-leg1 'y1 (- HIP-Y bounce-offset) HIP-Y 0.4 :ease 'quad-in :delay 0.4 :repeat 'infinite)
(animate jump-r-leg1 'y2 HIP-Y (- HIP-Y bounce-offset) 0.4 :ease 'quad-out :repeat 'infinite)
(animate jump-r-leg1 'y2 (- HIP-Y bounce-offset) HIP-Y 0.4 :ease 'quad-in :delay 0.4 :repeat 'infinite)

(animate jump-l-leg2 'y1 HIP-Y (- HIP-Y bounce-offset) 0.4 :ease 'quad-out :repeat 'infinite)
(animate jump-l-leg2 'y1 (- HIP-Y bounce-offset) HIP-Y 0.4 :ease 'quad-in :delay 0.4 :repeat 'infinite)
(animate jump-l-leg2 'y2 (- GROUND-Y 10) (- (- GROUND-Y bounce-offset) 10) 0.4 :ease 'quad-out :repeat 'infinite)
(animate jump-l-leg2 'y2 (- (- GROUND-Y bounce-offset) 10) (- GROUND-Y 10) 0.4 :ease 'quad-in :delay 0.4 :repeat 'infinite)

(animate jump-r-leg2 'y1 HIP-Y (- HIP-Y bounce-offset) 0.4 :ease 'quad-out :repeat 'infinite)
(animate jump-r-leg2 'y1 (- HIP-Y bounce-offset) HIP-Y 0.4 :ease 'quad-in :delay 0.4 :repeat 'infinite)
(animate jump-r-leg2 'y2 (- GROUND-Y 10) (- (- GROUND-Y bounce-offset) 10) 0.4 :ease 'quad-out :repeat 'infinite)
(animate jump-r-leg2 'y2 (- (- GROUND-Y bounce-offset) 10) (- GROUND-Y 10) 0.4 :ease 'quad-in :delay 0.4 :repeat 'infinite)

(play)
(render "examples_output/stickman_v2_jump.gif" :fps 15)
(println "  stickman_v2_jump.gif")


;; ======================================================================
;; 8. 四格序列图（优化版）
;; ======================================================================
(println "\n--- 8. 四格序列 ---")

(canvas 800 280 :bg "#fafafa")

;; 分格线
(add (line 200 0 200 280 :stroke "#ddd" :stroke-width 1))
(add (line 400 0 400 280 :stroke "#ddd" :stroke-width 1))
(add (line 600 0 600 280 :stroke "#ddd" :stroke-width 1))

;; 各区域分别绘制（偏移 CX）
(define (draw-in-column col-offset pose)
  (add (line (+ col-offset 30) GROUND-Y (+ col-offset 170) GROUND-Y :stroke "#bbb" :stroke-width 1))
  (add (ellipse (+ col-offset 100) (+ GROUND-Y 6) 40 4 :fill "rgba(0,0,0,0.06)"))
  
  (define local-cx (+ col-offset 100))
  
  ;; 头
  (add (circle local-cx HEAD-CY HEAD-R :fill "none" :stroke STROKE :stroke-width SW))
  (add (circle (- local-cx 5) (- HEAD-CY 2) 1.5 :fill STROKE))
  (add (circle (+ local-cx 5) (- HEAD-CY 2) 1.5 :fill STROKE))
  (add (line (- local-cx 5) (+ HEAD-CY 3) local-cx (+ HEAD-CY 6) :stroke STROKE :stroke-width 1.5))
  (add (line local-cx (+ HEAD-CY 6) (+ local-cx 5) (+ HEAD-CY 3) :stroke STROKE :stroke-width 1.5))
  
  ;; 躯干
  (add (line local-cx NECK-Y local-cx HIP-Y :stroke STROKE :stroke-width SW))
  (add (circle local-cx SHOULDER-Y JOINT-R :fill STROKE))
  (add (circle local-cx HIP-Y JOINT-R :fill STROKE))
  
  ;; 四肢按姿态
  (if (= pose 'standing)
    (begin
      (add (line local-cx SHOULDER-Y (- local-cx 28) (- HIP-Y 15) :stroke STROKE :stroke-width SW))
      (add (line local-cx SHOULDER-Y (+ local-cx 28) (- HIP-Y 15) :stroke STROKE :stroke-width SW))
      (add (line local-cx HIP-Y (- local-cx 22) GROUND-Y :stroke STROKE :stroke-width SW))
      (add (line local-cx HIP-Y (+ local-cx 22) GROUND-Y :stroke STROKE :stroke-width SW))
      (add (circle (- local-cx 28) (- HIP-Y 15) JOINT-R :fill STROKE))
      (add (circle (+ local-cx 28) (- HIP-Y 15) JOINT-R :fill STROKE))
      (add (circle (- local-cx 22) GROUND-Y FOOT-R :fill STROKE))
      (add (circle (+ local-cx 22) GROUND-Y FOOT-R :fill STROKE)))
    
    (if (= pose 'running)
      (begin
        (add (line local-cx SHOULDER-Y (- local-cx 38) (- HIP-Y 25) :stroke STROKE :stroke-width SW))
        (add (line local-cx SHOULDER-Y (+ local-cx 30) (- HIP-Y 10) :stroke STROKE :stroke-width SW))
        (add (line local-cx HIP-Y (- local-cx 32) (+ GROUND-Y 2) :stroke STROKE :stroke-width SW))
        (add (line local-cx HIP-Y (+ local-cx 28) (- GROUND-Y 5) :stroke STROKE :stroke-width SW))
        (add (circle (- local-cx 38) (- HIP-Y 25) JOINT-R :fill STROKE))
        (add (circle (+ local-cx 30) (- HIP-Y 10) JOINT-R :fill STROKE))
        (add (circle (- local-cx 32) (+ GROUND-Y 2) FOOT-R :fill STROKE))
        (add (circle (+ local-cx 28) (- GROUND-Y 5) FOOT-R :fill STROKE)))
      
      (if (= pose 'jumping)
        (begin
          (add (line local-cx SHOULDER-Y (- local-cx 18) (- HEAD-CY 30) :stroke STROKE :stroke-width SW))
          (add (line local-cx SHOULDER-Y (+ local-cx 18) (- HEAD-CY 30) :stroke STROKE :stroke-width SW))
          (add (line local-cx HIP-Y (- local-cx 28) HIP-Y :stroke STROKE :stroke-width SW))
          (add (line local-cx HIP-Y (+ local-cx 28) HIP-Y :stroke STROKE :stroke-width SW))
          (add (line (- local-cx 28) HIP-Y (- local-cx 22) (- GROUND-Y 10) :stroke STROKE :stroke-width SW))
          (add (line (+ local-cx 28) HIP-Y (+ local-cx 22) (- GROUND-Y 10) :stroke STROKE :stroke-width SW))
          (add (circle (- local-cx 18) (- HEAD-CY 30) JOINT-R :fill STROKE))
          (add (circle (+ local-cx 18) (- HEAD-CY 30) JOINT-R :fill STROKE))
          (add (circle (- local-cx 28) HIP-Y JOINT-R :fill STROKE))
          (add (circle (+ local-cx 28) HIP-Y JOINT-R :fill STROKE))
          (add (circle (- local-cx 22) (- GROUND-Y 10) FOOT-R :fill STROKE))
          (add (circle (+ local-cx 22) (- GROUND-Y 10) FOOT-R :fill STROKE)))
        
        (if (= pose 'waving)
          (begin
            (add (line local-cx SHOULDER-Y (+ local-cx 35) (- HEAD-CY 25) :stroke STROKE :stroke-width SW))
            (add (line local-cx SHOULDER-Y (- local-cx 28) (- HIP-Y 15) :stroke STROKE :stroke-width SW))
            (add (line local-cx HIP-Y (- local-cx 22) GROUND-Y :stroke STROKE :stroke-width SW))
            (add (line local-cx HIP-Y (+ local-cx 22) GROUND-Y :stroke STROKE :stroke-width SW))
            (add (circle (+ local-cx 35) (- HEAD-CY 25) JOINT-R :fill STROKE))
            (add (circle (- local-cx 28) (- HIP-Y 15) JOINT-R :fill STROKE))
            (add (circle (- local-cx 22) GROUND-Y FOOT-R :fill STROKE))
            (add (circle (+ local-cx 22) GROUND-Y FOOT-R :fill STROKE))))))))

(draw-in-column 0 'standing)
(draw-in-column 200 'running)
(draw-in-column 400 'jumping)
(draw-in-column 600 'waving)

(add (text "站立" 80 260 :fill "#999" :size 15))
(add (text "跑步" 285 260 :fill "#999" :size 15))
(add (text "跳跃" 485 260 :fill "#999" :size 15))
(add (text "挥手" 690 260 :fill "#999" :size 15))

(render "examples_output/stickman_v2_sequence.png")
(println "  stickman_v2_sequence.png")


;; ======================================================================
;; 9. 团队合影
;; ======================================================================
(println "\n--- 9. 团队合影 ---")

(canvas 400 280 :bg "#f0f4fa")

;; 三个不同姿态
(define (draw-in-team x-pos pose)
  (define local-cx x-pos)
  (add (line (- local-cx 70) GROUND-Y (+ local-cx 70) GROUND-Y :stroke "#ccc" :stroke-width 1))
  
  (add (circle local-cx HEAD-CY HEAD-R :fill "none" :stroke STROKE :stroke-width SW))
  (add (circle (- local-cx 5) (- HEAD-CY 2) 1.5 :fill STROKE))
  (add (circle (+ local-cx 5) (- HEAD-CY 2) 1.5 :fill STROKE))
  (add (line (- local-cx 5) (+ HEAD-CY 3) local-cx (+ HEAD-CY 6) :stroke STROKE :stroke-width 1.5))
  (add (line local-cx (+ HEAD-CY 6) (+ local-cx 5) (+ HEAD-CY 3) :stroke STROKE :stroke-width 1.5))
  
  (add (line local-cx NECK-Y local-cx HIP-Y :stroke STROKE :stroke-width SW))
  (add (circle local-cx SHOULDER-Y JOINT-R :fill STROKE))
  (add (circle local-cx HIP-Y JOINT-R :fill STROKE))
  
  (if (= pose 'standing)
    (begin
      (add (line local-cx SHOULDER-Y (- local-cx 28) (- HIP-Y 15) :stroke STROKE :stroke-width SW))
      (add (line local-cx SHOULDER-Y (+ local-cx 28) (- HIP-Y 15) :stroke STROKE :stroke-width SW))
      (add (line local-cx HIP-Y (- local-cx 22) GROUND-Y :stroke STROKE :stroke-width SW))
      (add (line local-cx HIP-Y (+ local-cx 22) GROUND-Y :stroke STROKE :stroke-width SW))
      (add (circle (- local-cx 28) (- HIP-Y 15) JOINT-R :fill STROKE))
      (add (circle (+ local-cx 28) (- HIP-Y 15) JOINT-R :fill STROKE))
      (add (circle (- local-cx 22) GROUND-Y FOOT-R :fill STROKE))
      (add (circle (+ local-cx 22) GROUND-Y FOOT-R :fill STROKE)))
    (if (= pose 'waving)
      (begin
        (add (line local-cx SHOULDER-Y (+ local-cx 35) (- HEAD-CY 25) :stroke STROKE :stroke-width SW))
        (add (line local-cx SHOULDER-Y (- local-cx 28) (- HIP-Y 15) :stroke STROKE :stroke-width SW))
        (add (line local-cx HIP-Y (- local-cx 22) GROUND-Y :stroke STROKE :stroke-width SW))
        (add (line local-cx HIP-Y (+ local-cx 22) GROUND-Y :stroke STROKE :stroke-width SW))
        (add (circle (+ local-cx 35) (- HEAD-CY 25) JOINT-R :fill STROKE))
        (add (circle (- local-cx 28) (- HIP-Y 15) JOINT-R :fill STROKE))
        (add (circle (- local-cx 22) GROUND-Y FOOT-R :fill STROKE))
        (add (circle (+ local-cx 22) GROUND-Y FOOT-R :fill STROKE)))
      (if (= pose 'jumping)
        (begin
          (add (line local-cx SHOULDER-Y (- local-cx 18) (- HEAD-CY 30) :stroke STROKE :stroke-width SW))
          (add (line local-cx SHOULDER-Y (+ local-cx 18) (- HEAD-CY 30) :stroke STROKE :stroke-width SW))
          (add (line local-cx HIP-Y (- local-cx 28) HIP-Y :stroke STROKE :stroke-width SW))
          (add (line local-cx HIP-Y (+ local-cx 28) HIP-Y :stroke STROKE :stroke-width SW))
          (add (line (- local-cx 28) HIP-Y (- local-cx 22) (- GROUND-Y 10) :stroke STROKE :stroke-width SW))
          (add (line (+ local-cx 28) HIP-Y (+ local-cx 22) (- GROUND-Y 10) :stroke STROKE :stroke-width SW))
          (add (circle (- local-cx 18) (- HEAD-CY 30) JOINT-R :fill STROKE))
          (add (circle (+ local-cx 18) (- HEAD-CY 30) JOINT-R :fill STROKE))
          (add (circle (- local-cx 28) HIP-Y JOINT-R :fill STROKE))
          (add (circle (+ local-cx 28) HIP-Y JOINT-R :fill STROKE))
          (add (circle (- local-cx 22) (- GROUND-Y 10) FOOT-R :fill STROKE))
          (add (circle (+ local-cx 22) (- GROUND-Y 10) FOOT-R :fill STROKE))))))
)

(draw-in-team 75 'waving)
(draw-in-team 200 'standing)
(draw-in-team 325 'jumping)

(render "examples_output/stickman_v2_team.png")
(println "  stickman_v2_team.png")


(println "\n=== 全部渲染完成！ ===")