;; ============================================================
;; 04-scene.pml — scene declarative form
;;
;; Syntax: (scene width height [:bg color] [:output path]
;;           elements...)
;; Creates a canvas with background, adds elements, and
;; optionally renders to a file.
;;
;; Elements are plain graphic calls inside the scene body.
;; ============================================================
(set-backend! "skia")

;; Declarative scene: 500x300, 4 shapes, auto-rendered
(scene 500 300 :bg "#dfe6e9"
  (circle 120 150 70 :fill "#ff7675" :stroke "#d63031" :stroke-width 3)
  (rect   220 90 160 120 :fill "#74b9ff" :rx 12 :stroke "#0984e3" :stroke-width 3)
  (polygon (list (list 430 80) (list 490 220) (list 370 220))
           :fill "#a29bfe" :stroke "#6c5ce7" :stroke-width 3)
  :output "04-scene.png"
)

(println "scene rendered to 04-scene.png")
