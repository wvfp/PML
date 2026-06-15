;; PML Standard Library — sprites/character.pml
;; Character sprite component — assembles body, head, eyes, mouth, hair, outfit, weapon
;;
;; Usage:
;;   (character
;;     :body (body :skin "#fce4c8" :build 'average)
;;     :head (head :shape 'oval :skin "#fce4c8")
;;     :hair (hair :style 'long :color "#2c2c2c")
;;     :outfit (outfit :top 'hoodie :bottom 'pants)
;;     :weapon (weapon :type 'sword)
;;     :pose 'standing
;;     :direction 'front
;;     :expression 'neutral
;;     :style 'cel
;;     :scale 1.0)
;;
;; Parameters:
;;   :body       — body GraphicObject (default: auto-generated)
;;   :head       — head GraphicObject (default: auto-generated)
;;   :hair       — hair GraphicObject (default: medium black)
;;   :outfit     — outfit GraphicObject (optional)
;;   :weapon     — weapon GraphicObject (optional, placed at character's side)
;;   :pose       — standing | walking | running | sitting | action
;;   :direction  — front | back | left | right | front-left | front-right
;;   :expression — neutral | happy | angry | sad | surprised
;;   :style      — cel | pixel | flat (or custom StyleDescriptor)
;;   :palette    — palette name string
;;   :scale      — 0.1 to 10.0 (default: 1.0)

(provide character)
