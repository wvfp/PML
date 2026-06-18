;; PML Cityscape v2 — Configuration
;; 4800×1200 ultra-wide sunset urban panorama

(define CANVAS-W 4800)
(define CANVAS-H 1200)

;; ── Sky ────────────────────────────────────────────────────────────────
(define SKY-TOP "#0d0d2b")
(define SKY-MID "#c43a1a")
(define SKY-BOT "#ff8844")
(define SKY-HAZE "#ff883322")

;; ── Sun ────────────────────────────────────────────────────────────────
(define SUN-X 3800)
(define SUN-Y 300)
(define SUN-R 50)
(define SUN-CORE "#ffe866")
(define SUN-GLOW "#ff8833")
(define SUN-GLOW-BIG "#ff883322")

;; ── Clouds ─────────────────────────────────────────────────────────────
(define CLOUD-COLOR "#ffcc8844")
(define CLOUD-COUNT 16)

;; ── Ground levels (y‑order, top→bottom) ───────────────────────────────
(define GROUND-FAR 620)
(define GROUND-MID 720)
(define GROUND-NEAR 760)
(define SIDEWALK-Y 760)
(define SIDEWALK-H 30)
(define ROAD-Y 790)
(define ROAD-H 140)

;; ── Far skyline palettes ──────────────────────────────────────────────
(define FAR-1 "#151525")
(define FAR-2 "#18182a")
(define FAR-3 "#1a1a30")

;; ── Mid‑building palettes (zoned) ─────────────────────────────────────
(define BIZ-1 "#152540")
(define BIZ-2 "#1e3a5a")
(define BIZ-3 "#2a4a6a")
(define BIZ-GLASS "#4a8aaa")
(define BIZ-GLASS2 "#6aaacc")
(define BIZ-GLASS3 "#88bbdd")

(define MID-1 "#1e2a34")
(define MID-2 "#2a3840")
(define MID-3 "#354550")
(define MID-4 "#1a2a3a")

(define RES-1 "#1a2828")
(define RES-2 "#243330")
(define RES-3 "#2e3f3a")
(define RES-4 "#1a2222")

(define IND-1 "#181e24")
(define IND-2 "#202830")
(define IND-3 "#283038")
(define IND-4 "#303840")

;; ── Near‑building palettes ────────────────────────────────────────────
(define NEAR-BIZ-1 "#0d1b2a")
(define NEAR-BIZ-2 "#1b2838")
(define NEAR-BIZ-3 "#253545")
(define NEAR-MID-1 "#15202b")
(define NEAR-MID-2 "#1e2a38")
(define NEAR-MID-3 "#283545")
(define NEAR-RES-1 "#1a2828")
(define NEAR-RES-2 "#243330")
(define NEAR-RES-3 "#152020")
(define NEAR-IND-1 "#12181e")
(define NEAR-IND-2 "#1a2228")
(define NEAR-IND-3 "#222a30")

;; ── Windows ────────────────────────────────────────────────────────────
(define WIN-ON "#ffcc44")
(define WIN-DIM "#cc8833")
(define WIN-OFF "#152535")
(define WIN-BIZ "#88ccdd")
(define WIN-GLOW "#ffdd66")
(define WIN-W 4)
(define WIN-GAP 10)

;; ── Building profiles ──────────────────────────────────────────────────
(define BLDG-MIN-W 30)
(define BLDG-MAX-W 120)
(define BLDG-MIN-H 60)
(define BLDG-MAX-H 280)

;; ── Street ─────────────────────────────────────────────────────────────
(define ROAD-CLR "#3a3a3a")
(define LANE-CLR "#777777")
(define WALK-CLR "#8a8a7a")
(define CURB-CLR "#666655")

(define CROSSWALK-CLR "#ffffff")
(define CROSSWALK-W 40)
(define CROSSWALK-H 3)
(define CROSSWALK-GAP 6)

;; ── Street lamps ───────────────────────────────────────────────────────
(define LAMP-X-POSITIONS (list 100 350 600 900 1200 1500 1850 2200
                               2600 2950 3300 3650 4000 4350 4650))
(define POLE-CLR "#555555")
(define LAMP-GLOW "#ffdd8866")
(define LAMP-CORE "#ffddaa")
(define LAMP-Y 740)

;; ── Trees ──────────────────────────────────────────────────────────────
(define TREE-X-POSITIONS (list 250 500 750 1050 1350 1700 2050
                               2400 2750 3100 3450 3800 4150 4500))
(define TRUNK-CLR "#3a2510")
(define LEAF-CLR "#1a3a1a")
(define LEAF-CLR2 "#1a4a2a")

;; ── Pedestrians ────────────────────────────────────────────────────────
(define PERSON-H 22)
(define PERSON-W 6)
(define PERSON-CLR "#151515")
(define PERSON-CLR2 "#222222")
(define PERSON-Y 750)

;; ── Shop fronts ────────────────────────────────────────────────────────
(define SHOP-AWNING "#cc4433")
(define SHOP-AWNING2 "#dd6633")
(define SHOP-AWNING3 "#4488aa")
(define SHOP-SIGN-W 50)
(define SHOP-SIGN-H 12)
(define SHOP-SIGN-CLR "#ffddaa")
(define SHOP-Y 740)
(define SHOP-H 20)

;; ── Billboards ─────────────────────────────────────────────────────────
(define BILLBOARD-W 100)
(define BILLBOARD-H 50)
(define BILLBOARD-CLR "#2a2a4a")
(define BILLBOARD-FRAME "#444466")
(define NEON-PINK "#ff4488")
(define NEON-CYAN "#44ffcc")
(define NEON-AMBER "#ffaa44")

;; ── Vehicles ───────────────────────────────────────────────────────────
(define CAR-COLORS (list "#cc4433" "#4488cc" "#44aa66" "#dddddd" "#333333"
                         "#dd6633" "#8833aa" "#cc8844"))
(define CAR-W 30)
(define CAR-H 10)
(define CAR-Y 810)
(define CAR-ROOF-H 6)
(define BUS-W 50)
(define BUS-H 14)
(define BUS-CLR "#4488cc")
(define BUS-Y 805)

;; ── Export everything ──────────────────────────────────────────────────
(provide CANVAS-W CANVAS-H
         SKY-TOP SKY-MID SKY-BOT SKY-HAZE
         SUN-X SUN-Y SUN-R SUN-CORE SUN-GLOW SUN-GLOW-BIG
         CLOUD-COLOR CLOUD-COUNT
         GROUND-FAR GROUND-MID GROUND-NEAR
         SIDEWALK-Y SIDEWALK-H ROAD-Y ROAD-H
         FAR-1 FAR-2 FAR-3
         BIZ-1 BIZ-2 BIZ-3 BIZ-GLASS BIZ-GLASS2 BIZ-GLASS3
         MID-1 MID-2 MID-3 MID-4
         RES-1 RES-2 RES-3 RES-4
         IND-1 IND-2 IND-3 IND-4
         NEAR-BIZ-1 NEAR-BIZ-2 NEAR-BIZ-3
         NEAR-MID-1 NEAR-MID-2 NEAR-MID-3
         NEAR-RES-1 NEAR-RES-2 NEAR-RES-3
         NEAR-IND-1 NEAR-IND-2 NEAR-IND-3
         WIN-ON WIN-DIM WIN-OFF WIN-BIZ WIN-GLOW WIN-W WIN-GAP
         BLDG-MIN-W BLDG-MAX-W BLDG-MIN-H BLDG-MAX-H
         ROAD-CLR LANE-CLR WALK-CLR CURB-CLR
         CROSSWALK-CLR CROSSWALK-W CROSSWALK-H CROSSWALK-GAP
         LAMP-X-POSITIONS POLE-CLR LAMP-GLOW LAMP-CORE LAMP-Y
         TREE-X-POSITIONS TRUNK-CLR LEAF-CLR LEAF-CLR2
         PERSON-H PERSON-W PERSON-CLR PERSON-CLR2 PERSON-Y
         SHOP-AWNING SHOP-AWNING2 SHOP-AWNING3
         SHOP-SIGN-W SHOP-SIGN-H SHOP-SIGN-CLR SHOP-Y SHOP-H
         BILLBOARD-W BILLBOARD-H BILLBOARD-CLR BILLBOARD-FRAME
         NEON-PINK NEON-CYAN NEON-AMBER
         CAR-COLORS CAR-W CAR-H CAR-Y CAR-ROOF-H
         BUS-W BUS-H BUS-CLR BUS-Y)
