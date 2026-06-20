; 示例 4：every-frame
; 用 every-frame 钩子实现简单粒子轨迹：每帧在原点生成一个向外扩散的小圆。

(set-backend! "skia")
(canvas 400 400 :bg "#0B0C10")

; 存储粒子状态：每个元素是 (x y vx vy birth-frame)
(define particles '())
(define frame-count 0)

(define (add-particle t frame)
  (define angle (* t 4.0))
  (define speed (+ 1.0 (* (modulo (floor (* t 100)) 5) 0.5)))
  (list (* 200 (cos angle))
        (* 200 (sin angle))
        (* speed (cos angle))
        (* speed (sin angle))
        frame))

(every-frame
  (lambda ()
    (define t (/ frame-count 30.0))
    (set! frame-count (+ frame-count 1))
    ; 每帧新增一个粒子
    (set! particles (cons (add-particle t frame-count) particles))
    ; 绘制所有粒子（老粒子自然淡出）
    (for-each
      (lambda (p)
        (define px (list-ref p 0))
        (define py (list-ref p 1))
        (define birth (list-ref p 4))
        (define age (- frame-count birth))
        (define x (+ 200 px))
        (define y (+ 200 py))
        (define alpha (max 0 (- 1.0 (* age 0.03))))
        (add (no-stroke (circle x y 4
                                :fill (color/rgba 100 200 255 (* alpha 255))))))
      particles)))

(render "04_every_frame.gif" :fps 30 :duration 3.0)
