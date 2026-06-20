; 项目 3：卡牌游戏 UI
; 展示完整游戏界面：面板、卡牌、按钮、血条、金币图标与阴影滤镜。

(set-backend! "skia")

(define comp (make-composition "card-game-ui" 640 400 :bg "#1A1A2E"))

; 背景面板
(define bg-panel (make-layer "bg"
                             (rect 0 0 640 400 :fill "#23243A" :rx 0)
                             :offset '(0 0)))

; 标题
(define title (make-layer "title"
                          (text 320 50 "Card Battle" :fill "#FFD700" :font-size 32 :align 'center)
                          :offset '(0 0)))

; 玩家血条
(define hp-bg (make-layer "hp-bg"
                          (rect 60 90 240 24 :fill "#3A3B52" :rx 12)
                          :offset '(0 0)))
(define hp-bar (make-layer "hp-bar"
                           (rect 60 90 180 24 :fill "#E74C3C" :rx 12)
                           :offset '(0 0)))
(define hp-icon (make-layer "hp-icon"
                            (icon :type 'heart :size 28 :color "#E74C3C")
                            :offset '(34 88)))

; 金币
(define coin-icon (make-layer "coin"
                              (icon :type 'star :size 24 :color "#FFD700")
                              :offset '(520 92)))
(define coin-text (make-layer "coin-text"
                              (text 548 92 "128" :fill "#FFFFFF" :font-size 18)
                              :offset '(0 0)))

; 卡牌 1
(define card-1 (make-layer "card-1"
                           (group (rect 0 0 120 160 :fill "#34495E" :stroke "#FFFFFF" :stroke-width 2 :rx 8)
                                  (circle 60 60 35 :fill "#E74C3C")
                                  (text 60 130 "Fire" :fill "#FFFFFF" :font-size 16 :align 'center))
                           :offset '(80 150)
                           :filter (drop-shadow :dx 4 :dy 6 :blur 8 :color "#000000" :opacity 0.5)))

; 卡牌 2
(define card-2 (make-layer "card-2"
                           (group (rect 0 0 120 160 :fill "#34495E" :stroke "#FFFFFF" :stroke-width 2 :rx 8)
                                  (circle 60 60 35 :fill "#3498DB")
                                  (text 60 130 "Ice" :fill "#FFFFFF" :font-size 16 :align 'center))
                           :offset '(260 150)
                           :filter (drop-shadow :dx 4 :dy 6 :blur 8 :color "#000000" :opacity 0.5)))

; 卡牌 3
(define card-3 (make-layer "card-3"
                           (group (rect 0 0 120 160 :fill "#34495E" :stroke "#FFFFFF" :stroke-width 2 :rx 8)
                                  (circle 60 60 35 :fill "#27AE60")
                                  (text 60 130 "Heal" :fill "#FFFFFF" :font-size 16 :align 'center))
                           :offset '(440 150)
                           :filter (drop-shadow :dx 4 :dy 6 :blur 8 :color "#000000" :opacity 0.5)))

; 按钮
(define btn (make-layer "btn"
                        (button :label "END TURN" :width 140 :height 42
                                :style 'rounded :color "#27AE60" :state 'normal)
                        :offset '(250 340)
                        :filter (drop-shadow :dx 2 :dy 4 :blur 6 :color "#000000" :opacity 0.4)))

(set! comp (composition-add comp bg-panel title hp-bg hp-bar hp-icon coin-icon coin-text card-1 card-2 card-3 btn))
(composition-render comp "03_card_game_ui.png")
