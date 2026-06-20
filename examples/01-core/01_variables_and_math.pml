; 示例 1：变量、算术与字符串操作
; 该示例只输出文本，不渲染图片。

(define width 800)
(define height 600)
(define pi 3.14159)

(define radius 10)
(define area (* pi radius radius))

(define greeting "面积 = ")
(define message (string-append greeting (number->string area)))

(println "宽度: " (number->string width))
(println "高度: " (number->string height))
(println "半径: " (number->string radius))
(println message)

; 混合运算
(define result (+ (* 2 width) (/ height 2)))
(println "2*width + height/2 = " (number->string result))
