;; PML Cityscape v2 — Vehicles
(import "config.pml" as cfg)

;; Car
(define (draw-car x y color)
  (add (rect x y cfg/CAR-W cfg/CAR-H :fill color :stroke "#222" :stroke-width 1))
  (add (rect (+ x 5) (- y cfg/CAR-ROOF-H) (- cfg/CAR-W 10) cfg/CAR-ROOF-H
        :fill color :stroke "#222" :stroke-width 1))
  ;; Windows
  (add (rect (+ x 8) (- y (- cfg/CAR-ROOF-H 2)) 8 4 :fill "#88bbff88"))
  (add (rect (+ x 18) (- y (- cfg/CAR-ROOF-H 2)) 8 4 :fill "#88bbff88"))
  ;; Wheels
  (add (rect (+ x 4) (+ y (- cfg/CAR-H 1)) 6 4 :fill "#222"))
  (add (rect (+ x (- cfg/CAR-W 10)) (+ y (- cfg/CAR-H 1)) 6 4 :fill "#222"))
  ;; Headlight
  (add (rect (+ x (- cfg/CAR-W 3)) (+ y 2) 3 3 :fill "#ffff8866")))

;; Bus with grid-of windows
(define (draw-bus x y color)
  (add (rect x y cfg/BUS-W cfg/BUS-H :fill color :stroke "#222" :stroke-width 1))
  (add (rect (+ x 3) (- y 6) (- cfg/BUS-W 6) 8 :fill color :stroke "#222" :stroke-width 1))
  ;; Side windows using grid-of
  (grid-of rect (i _) (6 1)
    :x (+ x 6 (* i 10)) :y (- y 4)
    :w 8 :h 6
    :fill "#88bbff88")
  ;; Wheels
  (add (rect (+ x 6) (+ y (- cfg/BUS-H 1)) 8 5 :fill "#222"))
  (add (rect (+ x (- cfg/BUS-W 14)) (+ y (- cfg/BUS-H 1)) 8 5 :fill "#222"))
  ;; Headlight
  (add (rect (+ x (- cfg/BUS-W 3)) (+ y 3) 3 5 :fill "#ffff8866")))

;; Truck (larger, boxy)
(define (draw-truck x y color)
  ;; Cab
  (add (rect x y 22 14 :fill color :stroke "#222" :stroke-width 1))
  ;; Cargo
  (add (rect (+ x 22) (- y 4) 40 18 :fill (color-blend color "#222" 0.3)
        :stroke "#222" :stroke-width 1))
  ;; Cab window
  (add (rect (+ x 3) (- y 2) 14 6 :fill "#88bbff88"))
  ;; Wheels
  (add (rect (+ x 3) (+ y 10) 6 5 :fill "#222"))
  (add (rect (+ x 14) (+ y 10) 6 5 :fill "#222"))
  (add (rect (+ x 28) (+ y 11) 6 5 :fill "#222"))
  (add (rect (+ x 54) (+ y 11) 6 5 :fill "#222")))

;; Place vehicles across 4800px
(define (draw-all-vehicles)
  ;; Lane 1 (top, rightward) — cars only
  (let ((lane-y (+ cfg/ROAD-Y 12)))
    (draw-car 150  lane-y "#4488cc")
    (draw-car 550  lane-y "#cc4433")
    (draw-car 950  lane-y "#44aa66")
    (draw-car 1350 lane-y "#dddddd")
    (draw-car 1750 lane-y "#333333")
    (draw-car 2150 lane-y "#dd6633")
    (draw-car 2550 lane-y "#8833aa")
    (draw-car 2950 lane-y "#cc8844")
    (draw-car 3350 lane-y "#4488cc")
    (draw-car 3750 lane-y "#cc4433")
    (draw-car 4150 lane-y "#44aa66")
    (draw-car 4550 lane-y "#dddddd"))
  ;; Lane 2 (middle) — mixed: buses + cars + trucks
  (let ((lane-y (+ cfg/ROAD-Y (quotient cfg/ROAD-H 3) 10)))
    (draw-bus 200   lane-y cfg/BUS-CLR)
    (draw-car 600   lane-y "#dddddd")
    (draw-bus 1050  lane-y "#cc4433")
    (draw-truck 1500 lane-y "#8833aa")
    (draw-car 2000  lane-y "#333333")
    (draw-bus 2500  lane-y "#3388aa")
    (draw-car 2950  lane-y "#cc8844")
    (draw-truck 3350 lane-y "#4488cc")
    (draw-bus 3900  lane-y cfg/BUS-CLR)
    (draw-car 4400  lane-y "#44aa66"))
  ;; Lane 3 (bottom, leftward — staggered)
  (let ((lane-y (+ cfg/ROAD-Y (* 2 (quotient cfg/ROAD-H 3)) 10)))
    (draw-car 350  lane-y "#dd6633")
    (draw-car 750  lane-y "#8833aa")
    (draw-truck 1200 lane-y "#cc4433")
    (draw-car 1700 lane-y "#cc8844")
    (draw-car 2100 lane-y "#4488cc")
    (draw-bus 2600  lane-y "#aa4433")
    (draw-car 3100 lane-y "#44aa66")
    (draw-car 3500 lane-y "#dddddd")
    (draw-truck 4000 lane-y "#333333")
    (draw-car 4500 lane-y "#dd6633")))

(provide draw-all-vehicles)
