;; PML Cityscape v2 — Sky, Sun, Clouds
(import "config.pml" as cfg)

(define (draw-sky)
  ;; Sunset gradient — gradient shorthand with opacity haze
  (add (rect 0 0 cfg/CANVAS-W 500
        :fill (gradient 90 cfg/SKY-TOP cfg/SKY-MID cfg/SKY-BOT)))
  ;; Atmospheric haze overlay near horizon
  (add (rect 0 380 cfg/CANVAS-W 120
        :fill (color-alpha cfg/SKY-BOT 0.15))))

(define (draw-sun)
  ;; Outer glow — large faint (opacity via keyword)
  (add (circle cfg/SUN-X cfg/SUN-Y (* cfg/SUN-R 4.0) :fill cfg/SUN-GLOW :opacity 0.15))
  ;; Mid glow
  (add (circle cfg/SUN-X cfg/SUN-Y (* cfg/SUN-R 2.0) :fill cfg/SUN-GLOW :opacity 0.4))
  ;; Core
  (add (circle cfg/SUN-X cfg/SUN-Y cfg/SUN-R :fill cfg/SUN-CORE :opacity 0.95))
  ;; Glow streak — horizontal light spread
  (add (ellipse cfg/SUN-X cfg/SUN-Y (* cfg/SUN-R 8) (* cfg/SUN-R 1.5)
        :fill cfg/SUN-GLOW :opacity 0.08)))

(define (draw-clouds)
  ;; Procedural clouds spread across 4800px width
  (add (ellipse 250 160 160 30 :fill cfg/CLOUD-COLOR :opacity 0.6))
  (add (ellipse 380 150 90 22 :fill (color-alpha "#ffcc44" 0.15)))
  (add (ellipse 600 200 180 35 :fill cfg/CLOUD-COLOR :opacity 0.7))
  (add (ellipse 750 190 100 25 :fill (color-alpha "#ffcc44" 0.15)))
  (add (ellipse 1100 230 140 30 :fill cfg/CLOUD-COLOR :opacity 0.5))
  (add (ellipse 1220 220 80 20 :fill (color-alpha "#ff8844" 0.2)))
  (add (ellipse 1700 170 120 28 :fill cfg/CLOUD-COLOR :opacity 0.6))
  (add (ellipse 1800 160 70 18 :fill (color-alpha "#ff8844" 0.15)))
  (add (ellipse 2100 210 150 32 :fill cfg/CLOUD-COLOR :opacity 0.5))
  (add (ellipse 2230 200 80 22 :fill (color-alpha "#ffcc44" 0.2)))
  ;; Extra clouds for 4800 width — right side
  (add (ellipse 2800 180 130 28 :fill cfg/CLOUD-COLOR :opacity 0.5))
  (add (ellipse 2950 170 70 18 :fill (color-alpha "#ff8844" 0.15)))
  (add (ellipse 3300 220 160 35 :fill cfg/CLOUD-COLOR :opacity 0.6))
  (add (ellipse 3450 210 90 22 :fill (color-alpha "#ffcc44" 0.2)))
  (add (ellipse 4000 160 140 30 :fill cfg/CLOUD-COLOR :opacity 0.4))
  (add (ellipse 4150 150 80 20 :fill (color-alpha "#ff8844" 0.2))))

(provide draw-sky draw-sun draw-clouds)
