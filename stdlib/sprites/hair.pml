;; PML Standard Library — sprites/hair.pml
;; Hair component — 11 hairstyle variants
;;
;; Usage:
;;   (hair :style 'long :color "#2c2c2c" :length 1.0 :bangs #t :highlights #f)
;;
;; Parameters:
;;   :style     — short | medium | long | ponytail | twintails | spiky | bob |
;;                braid | bun | mohawk | bald
;;   :color     — hair color (any color value)
;;   :length    — 0.5 to 2.0 (default: 1.0)
;;   :bangs     — #t | #f (default: #t)
;;   :highlights — #t | #f (default: #f)

(provide hair)
