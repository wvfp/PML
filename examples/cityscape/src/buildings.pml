;; PML Cityscape v2 — Mid and Near Buildings
;; 4800×1200: zoned palettes, glass towers, rooftop details, billboards
(import "config.pml" as cfg)

(define G-MID cfg/GROUND-MID)
(define G-NEAR cfg/GROUND-NEAR)

;; =====================================================================
;; Window helpers
;; =====================================================================
(define (win-grid bx by cols rows fill)
  (grid-of rect (c r) (cols rows)
    :x (+ bx 4 (* c 10))
    :y (+ by 4 (* r 10))
    :w 4 :h 4
    :fill fill))

(define (glass-panel bx by w h fill1 fill2)
  (add (rect bx by w h :fill fill1))
  (add (rect (+ bx 4) (+ by 4) (quotient w 3) (quotient h 4)
        :fill fill2 :opacity 0.25)))

;; =====================================================================
;; Rooftop details
;; =====================================================================
(define (rooftop-tower bx by w)
  (let ((cx (+ bx (quotient w 2))))
    (add (rect (- cx 6) (- by 8) 12 8 :fill "#252525"))
    (add (rect (- cx 3) (- by 14) 6 6 :fill "#333333"))))

(define (rooftop-antenna bx by)
  (add (rect bx (- by 20) 2 20 :fill "#222222"))
  (add (rect (- bx 2) (- by 6) 6 2 :fill "#333333")))

(define (rooftop-ac bx by)
  (add (rect bx (- by 8) 16 8 :fill "#2a2a2a")))

;; =====================================================================
;; Billboard
;; =====================================================================
(define (billboard bx by w h frame-color sign-color neon-color)
  (add (rect bx by w h :fill sign-color))
  (add (rect bx by w h :fill "none" :stroke frame-color :stroke-width 1))
  (add (rect (+ bx 4) (+ by h -4) (- w 8) 2 :fill neon-color :opacity 0.6)))

;; =====================================================================
;; MID BUILDINGS — zoned
;; Left: Ind  |  Left-mid: Res  |  Center: Biz  |  Right-mid: Mid  |  Right: Mixed
;; =====================================================================
(define (draw-mid-buildings)
  ;; ── Zone Industrial (x: 0–1050) ────────────────────────────────────
  (add (rect 40  400 100 320 :fill cfg/IND-1))
  (add (rect 40  390 100 10  :fill cfg/IND-2))
  (add (rect 60  380 30  10  :fill cfg/IND-3))
  (add (rect 100 380 30  10  :fill cfg/IND-3))
  (win-grid 44 404 10 16 cfg/WIN-DIM)

  (add (rect 150 360 80  360 :fill cfg/IND-2))
  (add (rect 155 460 12 260 :fill cfg/IND-3))
  (rooftop-tower 180 360 80)
  (win-grid 154 364 8 18 cfg/WIN-OFF)

  (add (rect 240 380 110 340 :fill cfg/IND-3))
  (win-grid 244 384 11 17 cfg/WIN-DIM)
  (rooftop-antenna 300 380)

  (add (rect 360 340 90  380 :fill cfg/IND-1))
  (win-grid 364 344 9 19 cfg/WIN-OFF)

  (add (rect 460 370 100 350 :fill cfg/IND-4))
  (win-grid 464 374 10 17 cfg/WIN-DIM)

  (add (rect 570 350 80  370 :fill cfg/IND-2))
  (win-grid 574 354 8 18 cfg/WIN-OFF)

  (add (rect 660 390 110 330 :fill cfg/IND-3))
  (glass-panel 664 394 40 50 cfg/BIZ-GLASS cfg/BIZ-GLASS2)
  (win-grid 714 394 4 16 cfg/WIN-DIM)

  (add (rect 780 330 90  390 :fill cfg/IND-1))
  (win-grid 784 334 9 19 cfg/WIN-DIM)
  (rooftop-antenna 820 330)

  (add (rect 880 360 100 360 :fill cfg/IND-4))
  (win-grid 884 364 10 18 cfg/WIN-OFF)

  (add (rect 990 380 80  340 :fill cfg/IND-2))
  (win-grid 994 384 8 17 cfg/WIN-DIM)

  ;; ── Zone Residential (x: 1080–2150) ────────────────────────────────
  (add (rect 1080 400 90  320 :fill cfg/RES-1))
  (win-grid 1084 404 9 16 cfg/WIN-ON)

  (add (rect 1180 360 100 360 :fill cfg/RES-2))
  (win-grid 1184 364 10 18 cfg/WIN-DIM)
  (rooftop-ac 1240 360)

  (add (rect 1290 380 80  340 :fill cfg/RES-3))
  (win-grid 1294 384 8 17 cfg/WIN-ON)

  (add (rect 1380 340 110 380 :fill cfg/RES-4))
  (win-grid 1384 344 11 19 cfg/WIN-DIM)

  (add (rect 1500 390 90  330 :fill cfg/RES-1))
  (win-grid 1504 394 9 16 cfg/WIN-ON)
  (rooftop-tower 1545 390 90)

  (add (rect 1600 370 80  350 :fill cfg/RES-2))
  (win-grid 1604 374 8 17 cfg/WIN-DIM)

  (add (rect 1690 350 120 370 :fill cfg/RES-3))
  (win-grid 1694 354 12 18 cfg/WIN-ON)

  (add (rect 1820 400 90  320 :fill cfg/RES-4))
  (win-grid 1824 404 9 16 cfg/WIN-DIM)

  (add (rect 1920 360 100 360 :fill cfg/RES-1))
  (win-grid 1924 364 10 18 cfg/WIN-ON)
  (rooftop-antenna 1970 360)

  (add (rect 2030 380 80  340 :fill cfg/RES-2))
  (win-grid 2034 384 8 17 cfg/WIN-DIM)

  (add (rect 2120 340 110 380 :fill cfg/RES-3))
  (win-grid 2124 344 11 19 cfg/WIN-ON)

  ;; ── Zone Business District (x: 2240–3200) ──────────────────────────
  (add (rect 2240 300 90  420 :fill cfg/BIZ-1))
  (glass-panel 2244 304 82 412 cfg/BIZ-GLASS cfg/BIZ-GLASS3)
  (rooftop-antenna 2285 300)

  (add (rect 2340 240 110 480 :fill cfg/BIZ-2))
  (glass-panel 2344 244 102 472 cfg/BIZ-GLASS2 cfg/BIZ-GLASS3)
  (billboard 2370 380 50 30 cfg/BILLBOARD-FRAME cfg/BILLBOARD-CLR cfg/NEON-CYAN)

  (add (rect 2460 350 90  370 :fill cfg/BIZ-3))
  (add (rect 2470 320 70  30  :fill cfg/BIZ-1))
  (add (rect 2480 300 50  20  :fill cfg/BIZ-2))
  (win-grid 2464 354 9 18 cfg/WIN-BIZ)

  (add (rect 2560 280 100 440 :fill cfg/BIZ-1))
  (glass-panel 2564 284 92 432 cfg/BIZ-GLASS cfg/BIZ-GLASS2)
  (rooftop-tower 2610 280 100)

  (add (rect 2670 370 80  350 :fill cfg/BIZ-2))
  (win-grid 2674 374 8 17 cfg/WIN-BIZ)
  (billboard 2685 420 40 20 cfg/BILLBOARD-FRAME cfg/BILLBOARD-CLR cfg/NEON-PINK)

  (add (rect 2760 320 90  400 :fill cfg/BIZ-3))
  (glass-panel 2764 324 82 392 cfg/BIZ-GLASS3 cfg/BIZ-GLASS)

  (add (rect 2860 360 80  360 :fill cfg/BIZ-1))
  (add (rect 2870 330 60  30  :fill cfg/BIZ-2))
  (win-grid 2864 364 8 18 cfg/WIN-BIZ)

  (add (rect 2950 290 110 430 :fill cfg/BIZ-2))
  (glass-panel 2954 294 102 422 cfg/BIZ-GLASS cfg/BIZ-GLASS2)
  (rooftop-antenna 3000 290)

  ;; ── Zone Midtown (x: 3070–3950) ────────────────────────────────────
  (add (rect 3070 350 100 370 :fill cfg/MID-1))
  (win-grid 3074 354 10 18 cfg/WIN-ON)
  (rooftop-ac 3130 350)

  (add (rect 3180 380 90  340 :fill cfg/MID-2))
  (win-grid 3184 384 9 17 cfg/WIN-DIM)

  (add (rect 3280 330 110 390 :fill cfg/MID-3))
  (win-grid 3284 334 11 19 cfg/WIN-ON)

  (add (rect 3400 370 80  350 :fill cfg/MID-4))
  (win-grid 3404 374 8 17 cfg/WIN-DIM)
  (rooftop-tower 3440 370 80)

  (add (rect 3490 340 100 380 :fill cfg/MID-1))
  (win-grid 3494 344 10 19 cfg/WIN-ON)

  (add (rect 3600 390 90  330 :fill cfg/MID-2))
  (win-grid 3604 394 9 16 cfg/WIN-DIM)

  (add (rect 3700 360 80  360 :fill cfg/MID-3))
  (win-grid 3704 364 8 18 cfg/WIN-ON)
  (rooftop-antenna 3740 360)

  (add (rect 3790 320 110 400 :fill cfg/MID-4))
  (win-grid 3794 324 11 20 cfg/WIN-DIM)
  (billboard 3820 450 50 25 cfg/BILLBOARD-FRAME cfg/BILLBOARD-CLR cfg/NEON-AMBER)

  ;; ── Zone Mixed (x: 4000–4800) ──────────────────────────────────────
  (add (rect 4010 350 100 370 :fill cfg/BIZ-1))
  (win-grid 4014 354 10 18 cfg/WIN-BIZ)

  (add (rect 4120 390 80  330 :fill cfg/RES-3))
  (win-grid 4124 394 8 16 cfg/WIN-ON)
  (rooftop-ac 4160 390)

  (add (rect 4210 340 110 380 :fill cfg/MID-2))
  (win-grid 4214 344 11 19 cfg/WIN-DIM)

  (add (rect 4330 370 90  350 :fill cfg/IND-1))
  (win-grid 4334 374 9 17 cfg/WIN-OFF)

  (add (rect 4430 360 80  360 :fill cfg/BIZ-2))
  (glass-panel 4434 364 72 352 cfg/BIZ-GLASS cfg/BIZ-GLASS3)

  (add (rect 4520 380 100 340 :fill cfg/RES-1))
  (win-grid 4524 384 10 17 cfg/WIN-ON)

  (add (rect 4630 350 90  370 :fill cfg/MID-3))
  (win-grid 4634 354 9 18 cfg/WIN-DIM)
  (rooftop-antenna 4675 350)

  (add (rect 4720 400 80  320 :fill cfg/IND-2))
  (win-grid 4724 404 8 16 cfg/WIN-OFF))

;; =====================================================================
;; NEAR BUILDINGS — wider, shop fronts at street level
;; =====================================================================
(define (draw-near-buildings)
  ;; ── Left industrial edge ───────────────────────────────────────────
  (add (rect 20  450 130 310 :fill cfg/NEAR-IND-1))
  (win-grid 24 454 13 15 cfg/WIN-DIM)
  (billboard 35 480 100 45 cfg/BILLBOARD-FRAME cfg/BILLBOARD-CLR cfg/NEON-PINK)

  (add (rect 160 430 100 330 :fill cfg/NEAR-IND-2))
  (win-grid 164 434 10 16 cfg/WIN-OFF)

  (add (rect 270 460 110 300 :fill cfg/NEAR-IND-3))
  (win-grid 274 464 11 15 cfg/WIN-DIM)
  (rooftop-tower 320 460 110)

  ;; ── Left‑residential ──────────────────────────────────────────────
  (add (rect 390 440 90  320 :fill cfg/NEAR-RES-1))
  (win-grid 394 444 9 16 cfg/WIN-ON)

  (add (rect 490 420 120 340 :fill cfg/NEAR-RES-2))
  (win-grid 494 424 12 17 cfg/WIN-DIM)

  (add (rect 620 470 80  290 :fill cfg/NEAR-RES-3))
  (win-grid 624 474 8 14 cfg/WIN-ON)

  ;; ── Left‑mid mixed ────────────────────────────────────────────────
  (add (rect 710 430 110 330 :fill cfg/NEAR-MID-1))
  (win-grid 714 434 11 16 cfg/WIN-ON)
  (rooftop-ac 770 430)

  (add (rect 830 460 90  300 :fill cfg/NEAR-MID-2))
  (win-grid 834 464 9 15 cfg/WIN-DIM)

  (add (rect 930 400 130 360 :fill cfg/NEAR-MID-3))
  (win-grid 934 404 13 18 cfg/WIN-ON)

  ;; ── Center business ───────────────────────────────────────────────
  (add (rect 1070 440 100 320 :fill cfg/NEAR-BIZ-1))
  (glass-panel 1074 444 92 312 cfg/BIZ-GLASS cfg/BIZ-GLASS2)
  (billboard 1090 490 60 30 cfg/BILLBOARD-FRAME cfg/BILLBOARD-CLR cfg/NEON-CYAN)

  (add (rect 1180 420 110 340 :fill cfg/NEAR-BIZ-2))
  (win-grid 1184 424 11 17 cfg/WIN-BIZ)

  (add (rect 1300 460 90  300 :fill cfg/NEAR-BIZ-3))
  (glass-panel 1304 464 82 292 cfg/BIZ-GLASS2 cfg/BIZ-GLASS3)

  ;; ── Center‑right midtown ──────────────────────────────────────────
  (add (rect 1400 430 100 330 :fill cfg/NEAR-MID-1))
  (win-grid 1404 434 10 16 cfg/WIN-ON)
  (rooftop-tower 1450 430 100)

  (add (rect 1510 460 80  300 :fill cfg/NEAR-MID-2))
  (win-grid 1514 464 8 15 cfg/WIN-DIM)

  (add (rect 1600 410 120 350 :fill cfg/NEAR-MID-3))
  (win-grid 1604 414 12 17 cfg/WIN-ON)

  ;; ── Right residential ─────────────────────────────────────────────
  (add (rect 1730 450 100 310 :fill cfg/NEAR-RES-1))
  (win-grid 1734 454 10 15 cfg/WIN-ON)

  (add (rect 1840 430 90  330 :fill cfg/NEAR-RES-2))
  (win-grid 1844 434 9 16 cfg/WIN-DIM)

  (add (rect 1940 470 110 290 :fill cfg/NEAR-RES-3))
  (win-grid 1944 474 11 14 cfg/WIN-ON)

  ;; ── Far‑right midtown ─────────────────────────────────────────────
  (add (rect 2060 440 100 320 :fill cfg/NEAR-MID-1))
  (win-grid 2064 444 10 16 cfg/WIN-DIM)
  (billboard 2080 490 60 30 cfg/BILLBOARD-FRAME cfg/BILLBOARD-CLR cfg/NEON-AMBER)

  (add (rect 2170 420 90  340 :fill cfg/NEAR-MID-2))
  (win-grid 2174 424 9 17 cfg/WIN-ON)

  (add (rect 2270 460 110 300 :fill cfg/NEAR-MID-3))
  (win-grid 2274 464 11 15 cfg/WIN-DIM)
  (rooftop-ac 2330 460)

  ;; ── Extended right (4800‑wide coverage) ────────────────────────────
  (add (rect 2390 440 100 320 :fill cfg/NEAR-BIZ-1))
  (glass-panel 2394 444 92 312 cfg/BIZ-GLASS cfg/BIZ-GLASS3)

  (add (rect 2500 460 90  300 :fill cfg/NEAR-RES-1))
  (win-grid 2504 464 9 15 cfg/WIN-ON)

  (add (rect 2600 420 110 340 :fill cfg/NEAR-IND-1))
  (win-grid 2604 424 11 17 cfg/WIN-OFF)

  (add (rect 2720 450 100 310 :fill cfg/NEAR-MID-2))
  (win-grid 2724 454 10 15 cfg/WIN-ON)

  (add (rect 2830 430 120 330 :fill cfg/NEAR-BIZ-2))
  (glass-panel 2834 434 112 322 cfg/BIZ-GLASS2 cfg/BIZ-GLASS)

  (add (rect 2960 470 90  290 :fill cfg/NEAR-RES-2))
  (win-grid 2964 474 9 14 cfg/WIN-DIM)

  (add (rect 3060 440 100 320 :fill cfg/NEAR-MID-3))
  (win-grid 3064 444 10 16 cfg/WIN-ON)
  (rooftop-tower 3110 440 100)

  (add (rect 3170 460 80  300 :fill cfg/NEAR-IND-2))
  (win-grid 3174 464 8 15 cfg/WIN-OFF)

  (add (rect 3260 420 110 340 :fill cfg/NEAR-BIZ-1))
  (win-grid 3264 424 11 17 cfg/WIN-BIZ)
  (billboard 3280 470 70 35 cfg/BILLBOARD-FRAME cfg/BILLBOARD-CLR cfg/NEON-PINK)

  (add (rect 3380 450 100 310 :fill cfg/NEAR-RES-3))
  (win-grid 3384 454 10 15 cfg/WIN-ON)

  (add (rect 3490 430 90  330 :fill cfg/NEAR-MID-1))
  (win-grid 3494 434 9 16 cfg/WIN-DIM)

  (add (rect 3590 470 110 290 :fill cfg/NEAR-BIZ-3))
  (glass-panel 3594 474 102 282 cfg/BIZ-GLASS3 cfg/BIZ-GLASS)

  (add (rect 3710 440 100 320 :fill cfg/NEAR-RES-1))
  (win-grid 3714 444 10 16 cfg/WIN-ON)

  (add (rect 3820 460 120 300 :fill cfg/NEAR-MID-2))
  (win-grid 3824 464 12 15 cfg/WIN-DIM)
  (rooftop-ac 3890 460)

  (add (rect 3950 430 90  330 :fill cfg/NEAR-IND-3))
  (win-grid 3954 434 9 16 cfg/WIN-OFF)

  (add (rect 4050 460 100 300 :fill cfg/NEAR-BIZ-1))
  (win-grid 4054 464 10 15 cfg/WIN-BIZ)

  (add (rect 4160 440 110 320 :fill cfg/NEAR-RES-2))
  (win-grid 4164 444 11 16 cfg/WIN-ON)

  (add (rect 4280 470 80  290 :fill cfg/NEAR-MID-3))
  (win-grid 4284 474 8 14 cfg/WIN-DIM)

  (add (rect 4370 430 100 330 :fill cfg/NEAR-BIZ-2))
  (glass-panel 4374 434 92 322 cfg/BIZ-GLASS cfg/BIZ-GLASS2)

  (add (rect 4480 460 90  300 :fill cfg/NEAR-RES-3))
  (win-grid 4484 464 9 15 cfg/WIN-ON)

  (add (rect 4580 440 110 320 :fill cfg/NEAR-MID-1))
  (win-grid 4584 444 11 16 cfg/WIN-DIM)

  (add (rect 4700 460 100 300 :fill cfg/NEAR-IND-1))
  (win-grid 4704 464 10 15 cfg/WIN-OFF))

(provide draw-mid-buildings draw-near-buildings)