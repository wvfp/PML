;; PML Standard Library — sprites/items.pml
;; Item components: weapon, potion, chest, generic-item
;;
;; weapon:
;;   (weapon :type 'sword :size 'medium :material 'steel :element 'none :ornament 'plain)
;;   Types: sword | axe | bow | staff | dagger | spear | gun
;;   Sizes: small | medium | large | legendary
;;   Materials: iron | steel | gold | crystal | wood | bone
;;   Elements: none | fire | ice | lightning | holy | dark
;;   Ornaments: plain | gem | rune | engraved
;;
;; potion:
;;   (potion :type 'health :container 'bottle :color "#e74c3c" :size 'medium)
;;   Types: health | mana | buff | poison | bomb
;;   Containers: bottle | flask | vial | jar
;;   Sizes: small | medium | large
;;
;; chest:
;;   (chest :type 'wooden :state 'closed :size 'medium)
;;   Types: wooden | iron | gold | crystal
;;   States: closed | open | locked
;;   Sizes: small | medium | large
;;
;; generic-item:
;;   (generic-item :name "key" :base-shape 'diamond :color "#f1c40f" :outline #t)
;;   Shapes: circle | rect | diamond | custom

(provide weapon potion chest generic-item)
