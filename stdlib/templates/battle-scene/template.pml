;; battle-scene — side-by-side battle scene with background
;;
;; (use-template "battle-scene" :hero-expression "angry" :enemy-type "dragon")

(define (battle-scene-main :key (hero-expression "happy") (enemy-type "goblin") (bg-type "plains") (width 400) (height 200))
  (scene width height :bg (cond
    ((equal? bg-type "plains") "#87ceeb")
    ((equal? bg-type "dungeon") "#2c2c2c")
    ((equal? bg-type "forest") "#2d5016")
    ("#87ceeb")))

  (layer :name "bg")
  (add (background :type bg-type :time 'day))

  (layer :name "ground")
  (add (rect 0 (- height 40) width 40 :fill "#5d4037"))

  (layer :name "characters")
  (row :gap 60 :align 'center
    (place (character :expression hero-expression :style 'cel) :align 'center)
    (place (character :style 'cel) :align 'center))

  (render (string-append "battle-" bg-type ".png")))
