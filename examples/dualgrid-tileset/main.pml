;; Dual-Grid Tileset System — PML implementation
;; Based on TapTap article
;; 4 terrains (grass/dirt/stone/water) x 16 transition patterns = 64 tiles
;; Output: 00_spritesheet.png / 01_demo_tilemap.png / 02_cross_terrain.png

(set-backend! "skia")
(println "=== Dual-Grid Tileset ===")

;; ──────────────────────────────────────────────────────────────────────────────
;; 1. Self-contained SkSL shader: procedural terrain textures + SDF + domain warp
;; ──────────────────────────────────────────────────────────────────────────────
(define sksl (shader "
uniform float u_bl, u_br, u_tl, u_tr;
uniform float u_seed, u_type;

float hash(float2 p) {
    return fract(sin(dot(p, float2(127.1, 311.7))) * 43758.5453123);
}

float fbm3(float2 p) {
    float v=0.0;
    {float2 a=floor(p);float2 b=fract(p);b=b*b*(3.0-2.0*b);v+=0.5*mix(mix(hash(a+float2(0,0)),hash(a+float2(1,0)),b.x),mix(hash(a+float2(0,1)),hash(a+float2(1,1)),b.x),b.y);}
    {p*=2.0;float2 a=floor(p);float2 b=fract(p);b=b*b*(3.0-2.0*b);v+=0.25*mix(mix(hash(a+float2(0,0)),hash(a+float2(1,0)),b.x),mix(hash(a+float2(0,1)),hash(a+float2(1,1)),b.x),b.y);}
    {p*=2.0;float2 a=floor(p);float2 b=fract(p);b=b*b*(3.0-2.0*b);v+=0.125*mix(mix(hash(a+float2(0,0)),hash(a+float2(1,0)),b.x),mix(hash(a+float2(0,1)),hash(a+float2(1,1)),b.x),b.y);}
    return v;
}

float fbm2(float2 p) {
    float v=0.0;
    {float2 a=floor(p);float2 b=fract(p);b=b*b*(3.0-2.0*b);v+=0.5*mix(mix(hash(a+float2(0,0)),hash(a+float2(1,0)),b.x),mix(hash(a+float2(0,1)),hash(a+float2(1,1)),b.x),b.y);}
    {p*=2.0;float2 a=floor(p);float2 b=fract(p);b=b*b*(3.0-2.0*b);v+=0.25*mix(mix(hash(a+float2(0,0)),hash(a+float2(1,0)),b.x),mix(hash(a+float2(0,1)),hash(a+float2(1,1)),b.x),b.y);}
    return v;
}

half3 terrain_color(float2 uv, float seed, float type) {
    float n=fbm2(uv*0.03+seed);
    float n2=fbm2(uv*0.06+seed+100.0);
    if (type<0.5) {
        float v=0.18+n*0.30+n2*0.08;
        return mix(half3(0.10,0.30,0.08), half3(0.25,0.65,0.20), v);
    } else if (type<1.5) {
        float v=0.30+n*0.25+n2*0.10;
        return mix(half3(0.30,0.18,0.08), half3(0.65,0.50,0.30), v);
    } else if (type<2.5) {
        float v=0.30+n*0.30+n2*0.15;
        return mix(half3(0.25,0.30,0.35), half3(0.55,0.58,0.62), v);
    } else {
        float v=0.20+n*0.25+n2*0.10;
        return mix(half3(0.05,0.15,0.40), half3(0.15,0.60,0.90), v);
    }
}

half4 main(float2 xy) {
    float ts=128.0, r=64.0, sw=4.0;

    float2 wuv=xy*0.025+u_seed;
    float wx=(fbm3(wuv)-0.5)*16.0;
    float wy=(fbm3(wuv+60.0)-0.5)*16.0;
    float2 wxy=clamp(xy+float2(wx,wy), 0.0, ts);

    half3 gcol=terrain_color(wxy, 10.0, 0.0);
    half3 tcol=terrain_color(wxy, u_seed+20.0, u_type);

    float d_bl=length(wxy-float2(0,ts))-r;
    float d_br=length(wxy-float2(ts,ts))-r;
    float d_tl=length(wxy-float2(0,0))-r;
    float d_tr=length(wxy-float2(ts,0))-r;

    bool bl=u_bl>0.5, br=u_br>0.5, tl=u_tl>0.5, tr=u_tr>0.5;

    float fill=1e10;
    if (bl) fill=min(fill,d_bl);
    if (br) fill=min(fill,d_br);
    if (tl) fill=min(fill,d_tl);
    if (tr) fill=min(fill,d_tr);

    if (bl&&br&&!tl&&!tr) fill=(ts-r)-wxy.y;
    if (tl&&tr&&!bl&&!br) fill=wxy.y-r;
    if (tl&&bl&&!tr&&!br) fill=wxy.x-r;
    if (tr&&br&&!tl&&!bl) fill=(ts-r)-wxy.x;

    float bsum=u_bl+u_br+u_tl+u_tr;
    if (bsum>2.5&&bsum<3.5) {
        if (!bl) fill=-d_bl; else if (!br) fill=-d_br;
        else if (!tl) fill=-d_tl; else fill=-d_tr;
    }
    if (bsum<0.5) fill=1.0;
    if (bsum>3.5) fill=-1.0;

    float a=1.0-smoothstep(-sw,sw,fill);
    return half4(mix(gcol,tcol,a), 1.0);
}
"))
(println "  shader compiled")


;; ──────────────────────────────────────────────────────────────────────────────
;; 2. Config & pattern data
;; ──────────────────────────────────────────────────────────────────────────────
(define TS 128.0)

;; 16 patterns as (bl, br, tl, tr)
(define patterns (list
  (list 0.0 0.0 0.0 0.0) (list 1.0 0.0 0.0 0.0)
  (list 0.0 1.0 0.0 0.0) (list 1.0 1.0 0.0 0.0)
  (list 0.0 0.0 1.0 0.0) (list 1.0 0.0 1.0 0.0)
  (list 0.0 1.0 1.0 0.0) (list 1.0 1.0 1.0 0.0)
  (list 0.0 0.0 0.0 1.0) (list 1.0 0.0 0.0 1.0)
  (list 0.0 1.0 0.0 1.0) (list 1.0 1.0 0.0 1.0)
  (list 0.0 0.0 1.0 1.0) (list 1.0 0.0 1.0 1.0)
  (list 0.0 1.0 1.0 1.0) (list 1.0 1.0 1.0 1.0)
))

;; Pattern index labels
(define plabels (list
  "0:EMPTY" "1:BL" "2:BR" "3:BL+BR"
  "4:TL" "5:TL+BL" "6:TL+BR" "7:TL+BL+BR"
  "8:TR" "9:TR+BL" "10:TR+BR" "11:TR+BL+BR"
  "12:TL+TR" "13:TL+TR+BL" "14:TL+TR+BR" "15:FULL"
))

;; Terrain names
(define tnames (list "Grass" "Dirt" "Stone" "Water"))


;; ──────────────────────────────────────────────────────────────────────────────
;; 3. Helper: render a tile at (x,y) with given pattern + terrain
;; ──────────────────────────────────────────────────────────────────────────────
(defn tile (x y bl br tl tr seed type-id)
  (define ud (make-uniforms bl br tl tr seed type-id))
  (define s (apply-uniforms sksl ud))
  (define go (apply-shader! (rect 0 0 TS TS :fill "#fff") s))
  (add (with-transform go (translate x y))))


;; ──────────────────────────────────────────────────────────────────────────────
;; 4. Spritesheet: 2048x512 (4 terrains x 16 patterns)
;; ──────────────────────────────────────────────────────────────────────────────
(println "  generating spritesheet...")

(canvas (* 16 TS) (* 4 TS) :bg "#1a1a2e")

(for-each (lambda (tidx)
  (for-each (lambda (pidx)
    (define p (get patterns pidx))
    (define col (mod pidx 4))
    (define row (quotient pidx 4))
    (tile (+ (* tidx 4 TS) (* col TS))
          (* row TS)
          (get p 0) (get p 1) (get p 2) (get p 3)
          (+ (* tidx 200) pidx) tidx))
    (range 0 16))

  ;; terrain label on the left
  (add (text (+ (* tidx 4 TS) -36) (- (* 2 TS) 6)
        (get tnames tidx) :fill "#aabbcc" :font-size 14 :align 'left)))
  (range 0 4))

;; pattern labels on top
(for-each (lambda (pidx)
  (add (text (+ (* pidx TS) (/ TS 2)) 3
        (get plabels pidx) :fill "#667788" :font-size 9 :align 'center)))
  (range 0 16))

(render "00_spritesheet.png")
(println "  saved 00_spritesheet.png")


;; ──────────────────────────────────────────────────────────────────────────────
;; 5. Demo tilemap: dirt patch in grass (640x640)
;; ──────────────────────────────────────────────────────────────────────────────
(println "  generating demo tilemap...")

;; Pre-computed 5x5 render grid: pattern indices for dirt-in-grass
(define dgrid (list
  (list  2  3  3  3  1)
  (list 10 15 15 15  5)
  (list 10 15 15 15  5)
  (list 10 15 15 15  5)
  (list  8 12 12 12  4)
))

(canvas (* 5 TS) (* 5 TS) :bg "#1a1a2e")

(for-each (lambda (row)
  (for-each (lambda (col)
    (define pat (get patterns (get (get dgrid row) col)))
    (tile (* col TS) (* row TS)
          (get pat 0) (get pat 1) (get pat 2) (get pat 3)
          (+ (* row 50) col) 1.0))
    (range 0 5)))
  (range 0 5))

(render "01_demo_tilemap.png")
(println "  saved 01_demo_tilemap.png")


;; ──────────────────────────────────────────────────────────────────────────────
;; 6. Cross-terrain demo: water + dirt + stone (640x640)
;; ──────────────────────────────────────────────────────────────────────────────
(println "  generating cross-terrain demo...")

;; 5x5 render grid: each cell = (type-id, pattern-idx)
;; Data grid (6x6):
;;   G G G G G G
;;   G W W D D G
;;   G W W D D G
;;   G D D S S G
;;   G D D S S G
;;   G G G G G G
(define cgrid (list
  (list (list 3.0  2) (list 3.0  3) (list 1.0  1) (list 1.0  3) (list 1.0  1))
  (list (list 3.0 10) (list 3.0 15) (list 1.0 15) (list 1.0 15) (list 1.0  5))
  (list (list 3.0 10) (list 3.0 15) (list 2.0 15) (list 2.0 15) (list 2.0  5))
  (list (list 1.0 10) (list 1.0 15) (list 2.0 15) (list 2.0 15) (list 2.0  5))
  (list (list 1.0  8) (list 1.0 12) (list 2.0 12) (list 2.0 12) (list 2.0  4))
))

(canvas (* 5 TS) (* 5 TS) :bg "#1a1a2e")

(for-each (lambda (row)
  (for-each (lambda (col)
    (define cell (get (get cgrid row) col))
    (define type-id (get cell 0))
    (define pat (get patterns (get cell 1)))
    (tile (* col TS) (* row TS)
          (get pat 0) (get pat 1) (get pat 2) (get pat 3)
          (+ (* row 50) col) type-id))
    (range 0 5)))
  (range 0 5))

(render "02_cross_terrain.png")
(println "  saved 02_cross_terrain.png")


(println "=== Done ===")
(println "Files: 00_spritesheet.png / 01_demo_tilemap.png / 02_cross_terrain.png")
