;; item-card — inventory item icon with label
;;
;; (use-template "item-card" :icon-type "potion" :rarity "rare" :name "Potion")

(define (item-card-main :key (icon-type "potion") (rarity "common") (name "Item") (width 64) (height 80))
  (sprite-canvas width height :bg "transparent" :anchor 'top-left)

  ;; Background panel based on rarity
  (column :gap 2 :padding 2 :align 'center
    (place (rect 0 0 width (- height 16) :fill (cond
      ((equal? rarity "rare") "#3498db")
      ((equal? rarity "epic") "#9b59b6")
      ((equal? rarity "legendary") "#f39c12")
      ("#ecf0f0"))) :width width)
    (place (icon :type icon-type) :align 'center)
    (place (label name :size 8) :align 'center))

  (render (string-append name ".png")))