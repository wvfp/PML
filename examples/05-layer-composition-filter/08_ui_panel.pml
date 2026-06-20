; Example 8: A game UI panel built from bitmap layers with blending.

(set-backend! "skia")

(define panel (make-composition "ui-panel" 400 300 :bg "#2C3E50"))

; Semi-transparent panel background
(define bg (make-layer "panel-bg"
                       (rect 0 0 400 300
                             :fill "#34495E"
                             :stroke "#5D6D7E"
                             :stroke-width 4
                             :rx 16 :ry 16)
                       :opacity 0.95))

; Header bar
(define header (make-layer "header"
                           (rect 0 0 400 56 :fill "#2980B9" :rx 16 :ry 16)
                           :opacity 1.0))

; Icon in the header (bitmap layer)
(define icon (bitmap-layer "../assets/coin.png"
                           :name "coin-icon"
                           :x 24 :y 12
                           :width 32 :height 32))

; Decorative crate image at the bottom
(define crate (bitmap-layer "../assets/crate.png"
                            :name "crate-thumb"
                            :x 280 :y 140
                            :width 80 :height 80))

; Character avatar with a drop shadow filter
(define avatar (bitmap-layer "../assets/character.png"
                             :name "avatar"
                             :x 40 :y 100
                             :width 96 :height 96
                             :filter (drop-shadow :dx 4 :dy 4 :blur 6 :color "#00000080")))

(set! panel (composition-add panel bg header icon crate avatar))
(composition-render panel "08_ui_panel.png")
