;; PML Cityscape v2 — Far Skyline Silhouettes
;; 4800×1200 ultra-wide skyline with landmarks
(import "config.pml" as cfg)

(define GROUND cfg/GROUND-FAR)

(define (draw-skyline)
  ;; Far-left industrial
  (add (rect 0    340 80  280 :fill cfg/FAR-1))
  (add (rect 60   300 90  320 :fill cfg/FAR-2))
  (add (rect 130  360 60  260 :fill cfg/FAR-1))
  (add (rect 170  310 100 310 :fill cfg/FAR-2))
  (add (rect 250  370 80  250 :fill cfg/FAR-1))
  ;; Antenna on left building
  (add (rect 266  280 4   40  :fill cfg/FAR-3))
  (add (rect 310  320 110 300 :fill cfg/FAR-2))
  (add (rect 400  360 70  260 :fill cfg/FAR-1))

  ;; Left — stepped building
  (add (rect 460  300 50  320 :fill cfg/FAR-2))
  (add (rect 465  270 40  350 :fill cfg/FAR-1))
  (add (rect 500  340 70  280 :fill cfg/FAR-2))
  ;; Spire landmark
  (add (rect 550  200 60  420 :fill cfg/FAR-1))
  (add (rect 575  140 10  60  :fill cfg/FAR-2 :opacity 0.8))
  (add (rect 580  120 2   20  :fill cfg/FAR-3))

  ;; Left-mid
  (add (rect 610  290 90  330 :fill cfg/FAR-2))
  (add (rect 680  330 80  290 :fill cfg/FAR-1))
  (add (rect 740  260 110 360 :fill cfg/FAR-3))
  (add (rect 830  310 70  310 :fill cfg/FAR-1))

  ;; Center-left — dome landmark
  (add (rect 890  250 100 370 :fill cfg/FAR-2))
  (add (rect 970  280 120 340 :fill cfg/FAR-1))
  ;; Small dome
  (add (ellipse 1010 255 40 25 :fill cfg/FAR-2))
  (add (ellipse 1010 255 20 15 :fill cfg/FAR-3))

  ;; Center — tallest cluster
  (add (rect 1080 200 80  420 :fill cfg/FAR-1))
  (add (rect 1140 160 100 460 :fill cfg/FAR-2))
  (add (rect 1220 210 130 410 :fill cfg/FAR-1))
  ;; Twin antennas
  (add (rect 1180 120 4   40  :fill cfg/FAR-3))
  (add (rect 1196 100 4   60  :fill cfg/FAR-3))

  ;; Center — stepped building
  (add (rect 1340 260 70  360 :fill cfg/FAR-2))
  (add (rect 1350 220 50  400 :fill cfg/FAR-1))
  (add (rect 1390 290 90  330 :fill cfg/FAR-2))
  (add (rect 1460 230 80  390 :fill cfg/FAR-3))
  ;; Antenna
  (add (rect 1495 180 4   50  :fill cfg/FAR-2))

  ;; Center-right
  (add (rect 1530 300 100 320 :fill cfg/FAR-1))
  (add (rect 1610 340 70  280 :fill cfg/FAR-2))
  (add (rect 1660 260 120 360 :fill cfg/FAR-1))
  (add (rect 1760 290 80  330 :fill cfg/FAR-2))

  ;; Right — angled/sloped silhouette
  (add (rect 1830 230 70  390 :fill cfg/FAR-3))
  (add (rect 1880 200 90  420 :fill cfg/FAR-2))
  (add (rect 1950 270 100 350 :fill cfg/FAR-1))
  ;; Small spire
  (add (rect 1915 150 8   50  :fill cfg/FAR-2))
  (add (rect 1918 130 2   20  :fill cfg/FAR-3))

  ;; Far-right
  (add (rect 2050 310 110 310 :fill cfg/FAR-2))
  (add (rect 2140 250 80  370 :fill cfg/FAR-1))
  (add (rect 2200 290 90  330 :fill cfg/FAR-2))
  (add (rect 2270 340 70  280 :fill cfg/FAR-1))
  (add (rect 2320 280 100 340 :fill cfg/FAR-3))
  (add (rect 2400 320 80  300 :fill cfg/FAR-2))

  ;; Extreme right
  (add (rect 2470 270 110 350 :fill cfg/FAR-1))
  (add (rect 2560 310 90  310 :fill cfg/FAR-2))
  (add (rect 2630 350 70  270 :fill cfg/FAR-1))
  (add (rect 2680 240 100 380 :fill cfg/FAR-2))
  (add (rect 2760 290 120 330 :fill cfg/FAR-1))
  ;; Antenna
  (add (rect 2820 240 4   50  :fill cfg/FAR-3))
  (add (rect 2880 320 80  300 :fill cfg/FAR-2))
  (add (rect 2940 260 90  360 :fill cfg/FAR-1))

  ;; Right edge
  (add (rect 3020 300 110 320 :fill cfg/FAR-2))
  (add (rect 3110 340 70  280 :fill cfg/FAR-1))
  (add (rect 3160 250 100 370 :fill cfg/FAR-3))
  ;; Dome
  (add (ellipse 3210 235 40 20 :fill cfg/FAR-2))
  (add (rect 3240 280 80  340 :fill cfg/FAR-1))
  (add (rect 3300 220 120 400 :fill cfg/FAR-2))
  (add (rect 3400 270 90  350 :fill cfg/FAR-1))

  ;; Far right edge
  (add (rect 3480 310 100 310 :fill cfg/FAR-2))
  (add (rect 3560 350 70  270 :fill cfg/FAR-1))
  (add (rect 3610 240 110 380 :fill cfg/FAR-3))
  (add (rect 3700 280 80  340 :fill cfg/FAR-2))
  (add (rect 3760 320 100 300 :fill cfg/FAR-1))
  (add (rect 3840 260 90  360 :fill cfg/FAR-2))
  ;; Spire
  (add (rect 3890 210 50  410 :fill cfg/FAR-1))
  (add (rect 3905 160 8   50  :fill cfg/FAR-2))
  (add (rect 3908 145 2   15  :fill cfg/FAR-3))

  ;; Final stretch
  (add (rect 3930 290 110 330 :fill cfg/FAR-2))
  (add (rect 4020 330 80  290 :fill cfg/FAR-1))
  (add (rect 4080 270 100 350 :fill cfg/FAR-3))
  (add (rect 4160 310 90  310 :fill cfg/FAR-2))
  (add (rect 4230 250 120 370 :fill cfg/FAR-1))
  (add (rect 4330 290 80  330 :fill cfg/FAR-2))
  ;; Antenna
  (add (rect 4360 240 4   50  :fill cfg/FAR-3))
  (add (rect 4400 320 100 300 :fill cfg/FAR-2))
  (add (rect 4480 270 90  350 :fill cfg/FAR-1))
  (add (rect 4550 340 110 280 :fill cfg/FAR-2))
  (add (rect 4640 300 80  320 :fill cfg/FAR-1))
  (add (rect 4700 250 100 370 :fill cfg/FAR-2)))

(provide draw-skyline)
