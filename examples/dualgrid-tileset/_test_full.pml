(set-backend! "skia")
(println "test")
(define s (shader "
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
"))
(println "ok")
