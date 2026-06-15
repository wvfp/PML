;; PML Standard Library — sprites/outfit.pml
;; Outfit/clothing component
;;
;; Usage:
;;   (outfit :top 'hoodie :bottom 'pants :shoes 'sneakers
;;           :color-top "#3498db" :color-bottom "#2c3e50" :detail 'plain)
;;
;; Parameters:
;;   :top         — t-shirt | jacket | hoodie | dress | armor | robe | suit | tank | sailor
;;   :bottom      — pants | skirt | shorts | long-skirt | armor
;;   :shoes       — boots | sneakers | sandals | heels | none
;;   :color-top   — upper garment color
;;   :color-bottom — lower garment color
;;   :detail      — plain | striped | pattern | badge

(provide outfit)
