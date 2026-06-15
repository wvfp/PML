;; PML Phase 4 Example — Extended Semantic Components
;; Demonstrates: outfit, items, UI widgets, scene elements

;; === Section 1: Item Gallery ===
(sprite-canvas 256 128)

(define sword (weapon :type 'sword :material 'gold :element 'fire :size 'large))
(define hp-potion (potion :type 'health :container 'flask))
(define gold-chest (chest :type 'gold :state 'open))
(define gem-item (generic-item :base-shape 'diamond :color '#e74c3c))

(add (group :transform (translate -80 -10) sword))
(add (group :transform (translate -20 10) hp-potion))
(add (group :transform (translate 30 10) gold-chest))
(add (group :transform (translate 80 0) gem-item))

(render "examples_output/items.png")


;; === Section 2: Character with Equipment ===
(sprite-canvas 128 128)

(define hero-outfit (outfit :top 'armor :bottom 'armor :shoes 'boots :detail 'pattern))
(define hero-weapon (weapon :type 'sword :material 'crystal :element 'holy :ornament 'gem))

(add (character
  :outfit hero-outfit
  :weapon hero-weapon
  :expression 'happy
  :scale 0.9
))

(render "examples_output/hero_equipped.png")


;; === Section 3: UI Widgets ===
(sprite-canvas 256 192)

(define title-panel (panel :width 200 :height 60 :title "Game Menu" :style 'ornate))
(add (group :transform (translate 0 -50) title-panel))

(define start-btn (button :label "Start" :width 100 :height 32 :style 'rounded :state 'normal))
(add (group :transform (translate 0 10) start-btn))

(define hp-bar (health-bar :value 0.75 :width 120 :height 14 :style 'gradient))
(add (group :transform (translate 0 45) hp-bar))

(define heart (icon :type 'heart :size 24 :style 'detailed))
(define star (icon :type 'star :size 24))
(add (group :transform (translate -30 70) heart))
(add (group :transform (translate 30 70) star))

(render "examples_output/ui_widgets.png")


;; === Section 4: Scene Tiles ===
(sprite-canvas 192 64)

(define grass-tile (tile :type 'grass :size 32))
(define stone-tile (tile :type 'stone :size 32))
(define water-tile (tile :type 'water :size 32))
(define brick-tile (tile :type 'brick :size 32))
(define wood-tile (tile :type 'wood :size 32))

(add (group :transform (translate -64 0) grass-tile))
(add (group :transform (translate -32 0) stone-tile))
(add (group :transform (translate 0 0) water-tile))
(add (group :transform (translate 32 0) brick-tile))
(add (group :transform (translate 64 0) wood-tile))

(render "examples_output/tiles.png")


;; === Section 5: Scene Decorations ===
(sprite-canvas 256 128)

(define big-tree (decoration :type 'tree :size 'large :season 'summer))
(define bush1 (decoration :type 'bush :size 'medium :season 'spring))
(define rock1 (decoration :type 'rock :size 'small))
(define lamp-post (decoration :type 'lamp :size 'medium))
(define torch1 (decoration :type 'torch))

(add (group :transform (translate -80 -10) big-tree))
(add (group :transform (translate -30 10) bush1))
(add (group :transform (translate 20 15) rock1))
(add (group :transform (translate 60 -10) lamp-post))
(add (group :transform (translate 90 0) torch1))

(render "examples_output/decorations.png")


;; === Section 6: Background Scene ===
(sprite-canvas 256 192)

(define bg (background :type 'forest :time 'dusk :weather 'fog :width 256 :height 192))
(add bg)

;; Add some decorations on top
(define fg-tree (decoration :type 'tree :size 'large :season 'autumn))
(add (group :transform (translate -40 30) fg-tree))

(define fg-bush (decoration :type 'bush :size 'small :season 'autumn))
(add (group :transform (translate 50 40) fg-bush))

(render "examples_output/scene_forest.png")
