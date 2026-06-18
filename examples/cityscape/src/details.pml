;; PML Cityscape v2 — Details: lamps, trees, pedestrians, shop fronts, street furniture
(import "config.pml" as cfg)

;; =====================================================================
;; Street lamp with glow (opacity)
;; =====================================================================
(define (draw-lamp x y)
  (add (rect x y 4 60 :fill cfg/POLE-CLR))
  (add (rect (- x 8) (- y 5) 20 3 :fill cfg/POLE-CLR))
  ;; Glow aura
  (add (circle x (- y 8) 14 :fill cfg/LAMP-GLOW :opacity 0.4))
  ;; Core
  (add (circle x (- y 8) 4 :fill cfg/LAMP-CORE :opacity 0.9)))

;; =====================================================================
;; Tree with layered foliage
;; =====================================================================
(define (draw-tree x y color)
  (add (rect (+ x 3) (+ y 15) 4 25 :fill cfg/TRUNK-CLR))
  (add (circle x (- y 5) 18 :fill color))
  (add (circle (- x 8) y 14 :fill color))
  (add (circle (+ x 8) y 14 :fill color)))

;; =====================================================================
;; Pedestrian silhouettes
;; =====================================================================
(define (person-standing x y color)
  ;; Head
  (add (circle x (- y 18) 3 :fill color))
  ;; Body
  (add (rect (- x 2) (- y 15) 4 10 :fill color))
  ;; Legs
  (add (rect (- x 3) (- y 5) 2 5 :fill color))
  (add (rect (+ x 1) (- y 5) 2 5 :fill color)))

(define (person-walking x y color)
  ;; Head
  (add (circle x (- y 18) 3 :fill color))
  ;; Body (leaning slightly)
  (add (rect (- x 1) (- y 15) 4 10 :fill color))
  ;; Legs (spread for walking)
  (add (rect (- x 4) (- y 5) 2 5 :fill color))
  (add (rect (+ x 2) (- y 5) 2 5 :fill color)))

;; =====================================================================
;; Shop front with awning and sign
;; =====================================================================
(define (shop-front x y w awning-color sign-text)
  ;; Awning
  (add (rect x y w 6 :fill awning-color))
  (add (rect x (+ y 2) w 2 :fill (color-blend awning-color "#000000" 0.2)))
  ;; Sign board
  (add (rect (+ x 4) (- y cfg/SHOP-SIGN-H) (- w 8) cfg/SHOP-SIGN-H :fill cfg/SHOP-SIGN-CLR))
  ;; Sign border
  (add (rect (+ x 4) (- y cfg/SHOP-SIGN-H) (- w 8) cfg/SHOP-SIGN-H
        :fill "none" :stroke awning-color :stroke-width 1)))

;; =====================================================================
;; Traffic light
;; =====================================================================
(define (traffic-light x y facing)
  ;; Pole
  (add (rect x y 3 50 :fill "#444444"))
  ;; Signal box
  (add (rect (- x 4) (- y 5) 11 24 :fill "#222222"))
  ;; Lamps (only one lit)
  (if (eq? facing 'ns)
    (begin
      (add (circle x (+ y 3) 3 :fill "#ff4444" :opacity 0.9))
      (add (circle x (- y 1) 3 :fill "#333333"))
      (add (circle x (- y 5) 3 :fill "#333333")))
    (begin
      (add (circle x (+ y 3) 3 :fill "#333333"))
      (add (circle x (- y 1) 3 :fill "#ffff44" :opacity 0.9))
      (add (circle x (- y 5) 3 :fill "#333333")))))

;; =====================================================================
;; Main draw functions
;; =====================================================================
(define (draw-all-lamps)
  ;; Upper sidewalk — 4800px spread
  (draw-lamp 200  (- cfg/SIDEWALK-Y 15))
  (draw-lamp 600  (- cfg/SIDEWALK-Y 15))
  (draw-lamp 1000 (- cfg/SIDEWALK-Y 15))
  (draw-lamp 1400 (- cfg/SIDEWALK-Y 15))
  (draw-lamp 1800 (- cfg/SIDEWALK-Y 15))
  (draw-lamp 2200 (- cfg/SIDEWALK-Y 15))
  (draw-lamp 2600 (- cfg/SIDEWALK-Y 15))
  (draw-lamp 3000 (- cfg/SIDEWALK-Y 15))
  (draw-lamp 3400 (- cfg/SIDEWALK-Y 15))
  (draw-lamp 3800 (- cfg/SIDEWALK-Y 15))
  (draw-lamp 4200 (- cfg/SIDEWALK-Y 15))
  (draw-lamp 4600 (- cfg/SIDEWALK-Y 15))
  ;; Lower sidewalk
  (draw-lamp 400  (+ cfg/ROAD-Y cfg/ROAD-H 15))
  (draw-lamp 800  (+ cfg/ROAD-Y cfg/ROAD-H 15))
  (draw-lamp 1200 (+ cfg/ROAD-Y cfg/ROAD-H 15))
  (draw-lamp 1600 (+ cfg/ROAD-Y cfg/ROAD-H 15))
  (draw-lamp 2000 (+ cfg/ROAD-Y cfg/ROAD-H 15))
  (draw-lamp 2400 (+ cfg/ROAD-Y cfg/ROAD-H 15))
  (draw-lamp 2800 (+ cfg/ROAD-Y cfg/ROAD-H 15))
  (draw-lamp 3200 (+ cfg/ROAD-Y cfg/ROAD-H 15))
  (draw-lamp 3600 (+ cfg/ROAD-Y cfg/ROAD-H 15))
  (draw-lamp 4000 (+ cfg/ROAD-Y cfg/ROAD-H 15))
  (draw-lamp 4400 (+ cfg/ROAD-Y cfg/ROAD-H 15)))

(define (draw-all-trees)
  ;; Upper sidewalk — interleaved with lamps
  (draw-tree 80   (- cfg/SIDEWALK-Y 10) cfg/LEAF-CLR)
  (draw-tree 450  (- cfg/SIDEWALK-Y 10) cfg/LEAF-CLR2)
  (draw-tree 850  (- cfg/SIDEWALK-Y 10) cfg/LEAF-CLR)
  (draw-tree 1250 (- cfg/SIDEWALK-Y 10) cfg/LEAF-CLR2)
  (draw-tree 1650 (- cfg/SIDEWALK-Y 10) cfg/LEAF-CLR)
  (draw-tree 2050 (- cfg/SIDEWALK-Y 10) cfg/LEAF-CLR2)
  (draw-tree 2450 (- cfg/SIDEWALK-Y 10) cfg/LEAF-CLR)
  (draw-tree 2850 (- cfg/SIDEWALK-Y 10) cfg/LEAF-CLR2)
  (draw-tree 3250 (- cfg/SIDEWALK-Y 10) cfg/LEAF-CLR)
  (draw-tree 3650 (- cfg/SIDEWALK-Y 10) cfg/LEAF-CLR2)
  (draw-tree 4050 (- cfg/SIDEWALK-Y 10) cfg/LEAF-CLR)
  (draw-tree 4450 (- cfg/SIDEWALK-Y 10) cfg/LEAF-CLR2)
  ;; Lower sidewalk
  (draw-tree 200  (+ cfg/ROAD-Y cfg/ROAD-H 10) cfg/LEAF-CLR)
  (draw-tree 600  (+ cfg/ROAD-Y cfg/ROAD-H 10) cfg/LEAF-CLR2)
  (draw-tree 1000 (+ cfg/ROAD-Y cfg/ROAD-H 10) cfg/LEAF-CLR)
  (draw-tree 1400 (+ cfg/ROAD-Y cfg/ROAD-H 10) cfg/LEAF-CLR2)
  (draw-tree 1800 (+ cfg/ROAD-Y cfg/ROAD-H 10) cfg/LEAF-CLR)
  (draw-tree 2200 (+ cfg/ROAD-Y cfg/ROAD-H 10) cfg/LEAF-CLR2)
  (draw-tree 2600 (+ cfg/ROAD-Y cfg/ROAD-H 10) cfg/LEAF-CLR)
  (draw-tree 3000 (+ cfg/ROAD-Y cfg/ROAD-H 10) cfg/LEAF-CLR2)
  (draw-tree 3400 (+ cfg/ROAD-Y cfg/ROAD-H 10) cfg/LEAF-CLR)
  (draw-tree 3800 (+ cfg/ROAD-Y cfg/ROAD-H 10) cfg/LEAF-CLR2)
  (draw-tree 4200 (+ cfg/ROAD-Y cfg/ROAD-H 10) cfg/LEAF-CLR)
  (draw-tree 4600 (+ cfg/ROAD-Y cfg/ROAD-H 10) cfg/LEAF-CLR2))

(define (draw-all-pedestrians)
  ;; Groups of pedestrians along upper sidewalk
  ;; Standing
  (person-standing 300  cfg/PERSON-Y cfg/PERSON-CLR)
  (person-standing 350  cfg/PERSON-Y cfg/PERSON-CLR2)
  (person-walking  1100 cfg/PERSON-Y cfg/PERSON-CLR)
  (person-walking  1150 cfg/PERSON-Y cfg/PERSON-CLR2)
  (person-standing 1850 cfg/PERSON-Y cfg/PERSON-CLR)
  (person-walking  3100 cfg/PERSON-Y cfg/PERSON-CLR)
  (person-walking  3150 cfg/PERSON-Y cfg/PERSON-CLR2)
  (person-standing 4050 cfg/PERSON-Y cfg/PERSON-CLR)
  ;; Upper sidewalk far
  (person-walking  700  cfg/PERSON-Y cfg/PERSON-CLR2)
  (person-standing 1500 cfg/PERSON-Y cfg/PERSON-CLR)
  (person-walking  2600 cfg/PERSON-Y cfg/PERSON-CLR2)
  (person-standing 3500 cfg/PERSON-Y cfg/PERSON-CLR)
  (person-walking  4400 cfg/PERSON-Y cfg/PERSON-CLR2)
  ;; Lower sidewalk
  (person-standing 550  (+ cfg/ROAD-Y cfg/ROAD-H 10) cfg/PERSON-CLR)
  (person-walking  1300 (+ cfg/ROAD-Y cfg/ROAD-H 10) cfg/PERSON-CLR2)
  (person-walking  1700 (+ cfg/ROAD-Y cfg/ROAD-H 10) cfg/PERSON-CLR)
  (person-standing 2200 (+ cfg/ROAD-Y cfg/ROAD-H 10) cfg/PERSON-CLR2)
  (person-walking  2900 (+ cfg/ROAD-Y cfg/ROAD-H 10) cfg/PERSON-CLR)
  (person-standing 3700 (+ cfg/ROAD-Y cfg/ROAD-H 10) cfg/PERSON-CLR2)
  (person-walking  4200 (+ cfg/ROAD-Y cfg/ROAD-H 10) cfg/PERSON-CLR))

(define (draw-shop-fronts)
  ;; Shop fronts along near building bases
  (shop-front 170 740 80  cfg/SHOP-AWNING "CAFE")
  (shop-front 400 740 70  cfg/SHOP-AWNING3 "MART")
  (shop-front 750 740 90  cfg/SHOP-AWNING2 "PHARM")
  (shop-front 1050 740 80 cfg/SHOP-AWNING "BAKE")
  (shop-front 1400 740 70 cfg/SHOP-AWNING3 "BANK")
  (shop-front 1850 740 80 cfg/SHOP-AWNING2 "EAT")
  (shop-front 2150 740 90 cfg/SHOP-AWNING "BOOKS")
  ;; Extended 4800
  (shop-front 2500 740 80 cfg/SHOP-AWNING3 "GYM")
  (shop-front 3100 740 70 cfg/SHOP-AWNING "BAR")
  (shop-front 3450 740 90 cfg/SHOP-AWNING2 "HOTEL")
  (shop-front 3900 740 80 cfg/SHOP-AWNING "CAFE")
  (shop-front 4250 740 70 cfg/SHOP-AWNING3 "MART")
  (shop-front 4550 740 80 cfg/SHOP-AWNING2 "PIZZA"))

(define (draw-traffic-lights)
  ;; Intersection traffic lights (near crosswalks)
  (traffic-light 750  cfg/SIDEWALK-Y 'ns)
  (traffic-light 800  cfg/SIDEWALK-Y 'ew)
  (traffic-light 1550 cfg/SIDEWALK-Y 'ns)
  (traffic-light 1600 cfg/SIDEWALK-Y 'ew)
  (traffic-light 2350 cfg/SIDEWALK-Y 'ns)
  (traffic-light 2400 cfg/SIDEWALK-Y 'ew)
  (traffic-light 3150 cfg/SIDEWALK-Y 'ns)
  (traffic-light 3200 cfg/SIDEWALK-Y 'ew)
  (traffic-light 3950 cfg/SIDEWALK-Y 'ns)
  (traffic-light 4000 cfg/SIDEWALK-Y 'ew))

(provide draw-all-lamps draw-all-trees draw-all-pedestrians
         draw-shop-fronts draw-traffic-lights)