(set-backend! "skia")
(println "test fbm inline...")
(define s (shader "
uniform float u_seed;

float hash(float2 p) {
    return fract(sin(dot(p, float2(127.1, 311.7))) * 43758.5453123);
}

float fbm(float2 p) {
    float v = 0.0;
    { float2 ip = floor(p); float2 fp = fract(p); fp = fp*fp*(3.0-2.0*fp); v += 0.5 * mix(mix(hash(ip+float2(0,0)), hash(ip+float2(1,0)), fp.x), mix(hash(ip+float2(0,1)), hash(ip+float2(1,1)), fp.x), fp.y); }
    { p *= 2.0; float2 ip = floor(p); float2 fp = fract(p); fp = fp*fp*(3.0-2.0*fp); v += 0.25 * mix(mix(hash(ip+float2(0,0)), hash(ip+float2(1,0)), fp.x), mix(hash(ip+float2(0,1)), hash(ip+float2(1,1)), fp.x), fp.y); }
    { p *= 2.0; float2 ip = floor(p); float2 fp = fract(p); fp = fp*fp*(3.0-2.0*fp); v += 0.125 * mix(mix(hash(ip+float2(0,0)), hash(ip+float2(1,0)), fp.x), mix(hash(ip+float2(0,1)), hash(ip+float2(1,1)), fp.x), fp.y); }
    return v;
}

half3 get_terrain(float2 uv, float seed, float type) {
    float n = fbm(uv * 0.025 + seed);
    float n2 = fbm(uv * 0.05 + seed + 100.0);
    if (type < 0.5) {
        float g = 0.18 + n*0.30 + n2*0.08;
        return mix(half3(0.1,0.3,0.1), half3(0.2,0.6,0.2), g);
    } else if (type < 1.5) {
        float d = 0.30 + n*0.25 + n2*0.10;
        return mix(half3(0.3,0.2,0.1), half3(0.6,0.5,0.3), d);
    }
    return half3(0.5,0.0,0.5);
}

half4 main(float2 xy) {
    half3 g = get_terrain(xy, 10.0, 0.0);
    half3 t = get_terrain(xy, u_seed+20.0, 1.0);
    return half4(mix(g, t, 0.3), 1.0);
}
"))
(println "compiled")
(canvas 128 128)
(define s2 (uniform-float s "u_seed" 42.0))
(define go (apply-shader! (rect 0 0 128 128 :fill "#fff") s2))
(add go)
(render "_test_fbm2.png")
(println "done")
