;; PML Standard Library — sprites/ui.pml
;; UI widget components: button, panel, health-bar, icon
;;
;; button:
;;   (button :label "Start" :width 120 :height 40 :style 'rounded :state 'normal
;;           :color "#3498db" :text-color "#FFFFFF")
;;   Styles: rounded | sharp | pixel | ornate
;;   States: normal | hover | pressed | disabled
;;
;; panel:
;;   (panel :width 200 :height 120 :style 'simple :title "Menu" :color "#2c3e50"
;;          :border-width 2)
;;   Styles: simple | ornate | pixel | glass
;;
;; health-bar:
;;   (health-bar :value 0.75 :width 150 :height 16 :color "#e74c3c"
;;               :bg-color "#2c3e50" :style 'gradient)
;;   Styles: flat | segmented | gradient
;;
;; icon:
;;   (icon :type 'heart :size 24 :color "#e74c3c" :style 'detailed)
;;   Types: heart | star | coin | gem | shield | skull
;;   Styles: flat | pixel | detailed

(provide button panel health-bar icon)
