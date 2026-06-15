;; PML Standard Library — sprites/body.pml
;; Body component — generates torso, neck, shoulders
;;
;; Usage:
;;   (body :height 160 :build 'average :skin "#fce4c8" :proportions 'anime)
;;
;; Parameters:
;;   :height      — 80 to 300 pixels (default: 160)
;;   :build       — slim | average | muscular | chubby
;;   :skin        — skin color (any color value)
;;   :proportions — realistic | anime | chibi

(provide body)
