; decorations.pml - 镜头左侧上方的三个装饰元素

(provide cluster)

; 粉旗
(define (pink-flag x y)
  (polygon (list (list x y)
                 (list (- x 55) (- y 15))
                 (list (- x 50) (- y 40))
                 (list x (- y 18)))
           :fill "#FF2E93"
           :stroke "#1A1A1A"
           :stroke-width 6))

; 绿叶
(define (green-leaf x y)
  (polygon (list (list x y)
                 (list (- x 25) (- y 25))
                 (list (- x 38) (- y 48))
                 (list (- x 18) (- y 60))
                 (list (- x 5)  (- y 38))
                 (list (- x 12) (- y 18)))
           :fill "#5BC85B"
           :stroke "#1A1A1A"
           :stroke-width 6))

; 蓝色水滴
(define (blue-droplet x y)
  (polygon (list (list x y)
                 (list (- x 35) (- y 45))
                 (list (- x 40) (- y 90))
                 (list (- x 22) (- y 120))
                 (list x (- y 135))
                 (list (+ x 22) (- y 120))
                 (list (+ x 40) (- y 90))
                 (list (+ x 35) (- y 45)))
           :fill "#2EC4E5"
           :stroke "#1A1A1A"
           :stroke-width 6))

(define (cluster)
  (group
    (pink-flag 200 210)
    (green-leaf 180 290)
    (blue-droplet 200 360)))
