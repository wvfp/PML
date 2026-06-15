;; PML Standard Library — sprites/styles.pml
;; Style system and palette reference
;;
;; Predefined styles:
;;   (use-style 'cel)   — anime cel-shaded: thick outlines, flat colors
;;   (use-style 'pixel) — pixel art: blocky, no anti-aliasing
;;   (use-style 'flat)  — flat design: thin outlines, solid colors
;;
;; Custom styles:
;;   (define-style "my-style"
;;     :outline-width 3
;;     :outline-color "#000000"
;;     :shading 'cel
;;     :anti-alias #f)
;;
;; Predefined palettes:
;;   "dark-hero"  — dark tones for heroic characters
;;   "warm-skin"  — warm skin tone palette
;;
;; Custom palettes:
;;   (define-palette "forest"
;;     :colors (list
;;       (list "skin" "#fce4c8")
;;       (list "hair" "#4a2c0a")
;;       (list "cloth" "#2e7d32")))
;;   (character :palette "forest")

(provide use-style define-style define-palette)
