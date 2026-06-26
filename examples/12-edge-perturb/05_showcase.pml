; ==========================================================================================================================================================================================================================================═
; Edge Perturbation — Full Showcase
; ------------------------------------------------------------------------------------------------------------------------------------------------------------─
; Creative exploration: different polygon types, seed variation, organic shapes,
; and layering perturbation with other polygon features.
; ==========================================================================================================================================================================================================================================═

(set-backend! "skia")
(canvas 640 480 :bg "#1a1a2e")

(define (label x y msg)
  (add (text x y msg :fill "#a0a0b0" :font-size 10)))

; ---- Helper: list of 4 colors ----------------------------------------------------------------------------------------------------
(define c4 '("#e74c3c" "#3498db" "#2ecc71" "#f39c12"))

; ---- 1. Seed exploration — 4 pentagons with different seeds ----------------------------------------
(define pent-pts '((60 60) (100 30) (140 60) (130 110) (70 110)))

(for-each
  (lambda (i)
    (let ((seed (* i 100))
          (pts (perturb-polygon pent-pts
                  :edge-noise 0.2 :edge-subdiv 4 :seed (* i 100))))
      (add (with-transform
             (polygon pts :fill (list-ref c4 i)
                      :stroke "#fff" :stroke-width 1)
             (translate (* i 155) 0)))
      (label (+ 10 (* i 155)) 130
             (string-append "Seed=" (number->string (* i 100))))))
  (range 0 4))

; ---- 2. Organic shapes — hexagon with strong noise + rounding --------------------------------─
(define hex-pts '((80 220) (140 190) (200 220) (200 280) (140 310) (80 280)))

; Exact hexagon
(add (polygon hex-pts :fill "#6c5ce7" :stroke "#a29bfe" :stroke-width 1))
(label 90 325 "Exact hexagon")

; Perturbed hexagon — strong noise
(define hex-noise (perturb-polygon hex-pts
                     :edge-noise 0.35 :edge-subdiv 6
                     :corner-radius 4.0
                     :seed 777))
(add (with-transform
       (polygon hex-noise :fill "#6c5ce7" :stroke "#a29bfe" :stroke-width 1)
       (translate 220 0)))
(label 310 325 "Strong noise + rounding")

; ---- 3. Natural stone outline — quadrilateral with very irregular edges ------------─
(define stone-pts '((40 400) (160 380) (200 440) (80 460)))

(define stone-rough (perturb-polygon stone-pts
                       :edge-noise 0.4 :edge-subdiv 6
                       :corner-radius 6.0
                       :seed 42))
(add (polygon stone-rough :fill "#8B4513" :stroke "#CD853F" :stroke-width 2))
(label 40 480 "Stone-like outline")

; ---- 4. Layered perturbation ----------------------------------------------------------------------------------------------------
(define base '((380 340) (480 340) (480 440) (380 440)))
(define layer1 (perturb-polygon base
                  :edge-noise 0.3 :edge-subdiv 5 :seed 111))
(define layer2 (perturb-polygon layer1
                  :edge-noise 0.1 :edge-subdiv 3 :seed 222))
(add (polygon layer2 :fill "#d35400" :stroke "#e67e22" :stroke-width 2))
(label 390 480 "Double-perturbed layer")

; ---- 5. Half-perturbed — mix exact and perturbed sides --------------------------------------------─
(define mixed (perturb-polygon '((500 100) (600 50) (620 150) (520 180))
                 :edge-noise '(0.3 0.0 0.2 0.0)
                 :edge-subdiv '(5 0 4 0)
                 :edge-mask '(#t #f #t #f)
                 :seed 55))
(add (polygon mixed :fill "#00b894" :stroke "#55efc4" :stroke-width 1))
(label 500 60 "Mixed edges")

(render "05_showcase.png")
