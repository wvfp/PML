;; PML Standard Library — sprites/scene.pml
;; Scene elements: tile, decoration, background
;;
;; tile:
;;   (tile :type 'grass :size 32 :variant 0 :edge 'none)
;;   Types: grass | stone | wood | sand | water | snow | brick
;;   Size: 8 to 128 (default: 32)
;;   Variant: 0 to 3
;;   Edge: none | top | bottom | left | right
;;
;; decoration:
;;   (decoration :type 'tree :size 'medium :season 'summer :variant 0)
;;   Types: tree | bush | rock | flower | mushroom | crate | barrel | torch |
;;          sign | fence | lamp
;;   Sizes: small | medium | large
;;   Seasons: spring | summer | autumn | winter
;;
;; background:
;;   (background :type 'forest :time 'dusk :weather 'fog :width 256 :height 256
;;               :parallax 1.0)
;;   Types: sky | forest | dungeon | town | ocean | mountain
;;   Times: day | dusk | night | dawn
;;   Weather: clear | cloudy | rain | snow | fog

(provide tile decoration background)
