(set-backend! "skia")
(println "testing fbm shader...")
(define s (shader "
uniform float u_seed;
uniform float u_type;

float hash(float2 p) {
    return fract(sin(dot(p, float2(127.1, 311.7))) * 43758.5453123);
}

float fbm(float2 p) {
    float v = 0.0, a = 0.5;
    for (int i = 0; i < 3; i++) {
        float2 ip = floor(p);
        float2 fp = fract(p);
        fp = fp * fp * (3.0 - 2.0 * fp);
        float n = mix(mix(hash(ip+float2(0,0)), hash(ip+float2(1,0)), fp.x),
                      mix(hash(ip+float2(0,1)), hash(ip+float2(1,1)), fp.x), fp.y);
        v += a * n; p *= 2.0; a *= 0.5;
    }
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
    } else if (type < 2.5) {
        float s = 0.30 + n*0.30 + n2*0.15;
        return mix(half3(0.2,0.3,0.3), half3(0.5,0.6,0.6), s);
    } else {
        float w = 0.20 + n*0.25 + n2*0.10;
        return mix(half3(0.0,0.1,0.4), half3(0.1,0.6,0.9), w);
    }
}

half4 main(float2 xy) {
    float2 wxy = xy;
    half3 grass = get_terrain(wxy, 10.0, 0.0);
    half3 terrain = get_terrain(wxy, u_seed + 20.0, u_type);
    return half4(mix(grass, terrain, 0.5), 1.0);
}
"))
(println "shader compiled OK")

(canvas 100 100)
(define s1 (uniform-float s "u_seed" 42.0))
(define s2 (uniform-float s1 "u_type" 1.0))
(define go (apply-shader! (rect 0 0 100 100 :fill "#fff") s2))
(add go)
(render "_test_fbm.png")
(println "done")
