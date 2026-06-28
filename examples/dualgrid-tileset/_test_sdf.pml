(set-backend! "skia")
(println "test full SDF shader...")
(define s (shader "
uniform float u_bl, u_br, u_tl, u_tr;
uniform float u_seed, u_type;

float hash(float2 p) {
    return fract(sin(dot(p, float2(127.1, 311.7))) * 43758.5453123);
}

float fbm(float2 p) {
    float v = 0.0;
    { float2 ip=floor(p); float2 fp=fract(p); fp=fp*fp*(3.0-2.0*fp); v+=0.5*mix(mix(hash(ip+float2(0,0)),hash(ip+float2(1,0)),fp.x),mix(hash(ip+float2(0,1)),hash(ip+float2(1,1)),fp.x),fp.y); }
    { p*=2.0; float2 ip=floor(p); float2 fp=fract(p); fp=fp*fp*(3.0-2.0*fp); v+=0.25*mix(mix(hash(ip+float2(0,0)),hash(ip+float2(1,0)),fp.x),mix(hash(ip+float2(0,1)),hash(ip+float2(1,1)),fp.x),fp.y); }
    { p*=2.0; float2 ip=floor(p); float2 fp=fract(p); fp=fp*fp*(3.0-2.0*fp); v+=0.125*mix(mix(hash(ip+float2(0,0)),hash(ip+float2(1,0)),fp.x),mix(hash(ip+float2(0,1)),hash(ip+float2(1,1)),fp.x),fp.y); }
    return v;
}

half3 get_terrain(float2 uv, float seed, float type) {
    float n = fbm(uv*0.025+seed);
    float n2 = fbm(uv*0.05+seed+100.0);
    if (type<0.5) { float g=0.18+n*0.30+n2*0.08; return mix(half3(0.12,0.35,0.10),half3(0.25,0.65,0.20),g); }
    else if (type<1.5) { float d=0.30+n*0.25+n2*0.10; return mix(half3(0.30,0.18,0.08),half3(0.65,0.50,0.30),d); }
    else if (type<2.5) { float s=0.30+n*0.30+n2*0.15; return mix(half3(0.25,0.30,0.35),half3(0.55,0.58,0.62),s); }
    else { float w=0.20+n*0.25+n2*0.10; return mix(half3(0.05,0.15,0.40),half3(0.15,0.60,0.90),w); }
}

half4 main(float2 xy) {
    float ts=128.0, r=64.0, sw=4.0;

    float2 wuv=xy*0.025+u_seed;
    float wx=(fbm(wuv)-0.5)*16.0, wy=(fbm(wuv+60.0)-0.5)*16.0;
    float2 wxy=clamp(xy+float2(wx,wy),0.0,ts);

    half3 gcol=get_terrain(wxy,10.0,0.0);
    half3 tcol=get_terrain(wxy,u_seed+20.0,u_type);

    float bl_s=length(wxy-float2(0,ts))-r;
    float br_s=length(wxy-float2(ts,ts))-r;
    float tl_s=length(wxy-float2(0,0))-r;
    float tr_s=length(wxy-float2(ts,0))-r;

    bool bl=u_bl>0.5, br=u_br>0.5, tl=u_tl>0.5, tr=u_tr>0.5;
    float fill=1e10;
    if (bl) fill=min(fill,bl_s);
    if (br) fill=min(fill,br_s);
    if (tl) fill=min(fill,tl_s);
    if (tr) fill=min(fill,tr_s);

    if (bl&&br&&!tl&&!tr) fill=(ts-r)-wxy.y;
    if (tl&&tr&&!bl&&!br) fill=wxy.y-r;
    if (tl&&bl&&!tr&&!br) fill=wxy.x-r;
    if (tr&&br&&!tl&&!bl) fill=(ts-r)-wxy.x;

    float bsum=u_bl+u_br+u_tl+u_tr;
    if (bsum>2.5&&bsum<3.5) {
        if (!bl) fill=-bl_s; else if (!br) fill=-br_s;
        else if (!tl) fill=-tl_s; else fill=-tr_s;
    }
    if (bsum<0.5) fill=1.0;
    if (bsum>3.5) fill=-1.0;

    float alpha=1.0-smoothstep(-sw,sw,fill);
    return half4(mix(gcol,tcol,alpha),1.0);
}
"))
(println "  compiled OK")

(define TS 128)
(define patterns (list
  (list 0.0 0.0 0.0 0.0) (list 1.0 0.0 0.0 0.0) (list 0.0 1.0 0.0 0.0) (list 1.0 1.0 0.0 0.0)
  (list 0.0 0.0 1.0 0.0) (list 1.0 0.0 1.0 0.0) (list 0.0 1.0 1.0 0.0) (list 1.0 1.0 1.0 0.0)
  (list 0.0 0.0 0.0 1.0) (list 1.0 0.0 0.0 1.0) (list 0.0 1.0 0.0 1.0) (list 1.0 1.0 0.0 1.0)
  (list 0.0 0.0 1.0 1.0) (list 1.0 0.0 1.0 1.0) (list 0.0 1.0 1.0 1.0) (list 1.0 1.0 1.0 1.0)
))

(canvas (* 4 TS) (* 4 TS) :bg "#222")

(dotimes (row 4)
  (dotimes (col 4)
    (define pidx (+ (* row 4) col))
    (define p (get patterns pidx))
    (define x (* col TS)) (define y (* row TS))
    (define s1 (uniform-float s "u_seed" (to-double pidx)))
    (define s2 (uniform-float s1 "u_type" 1.0))
    (define s3 (uniform-float s2 "u_bl" (get p 0)))
    (define s4 (uniform-float s3 "u_br" (get p 1)))
    (define s5 (uniform-float s4 "u_tl" (get p 2)))
    (define s6 (uniform-float s5 "u_tr" (get p 3)))
    (define go (apply-shader! (rect 0 0 TS TS :fill "#fff") s6))
    (add (with-transform go (translate (to-double x) (to-double y))))))

(println "  rendering...")
(render "_test_sdf.png")
(println "done")
