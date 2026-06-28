;; ═══════════════════════════════════════════════════════════════════════════════
;; 双网格（Dual-Grid）瓦片系统 — PML 复刻
;;
;; 基于 TapTap 文章「如何让嗒啦啦制作2D游戏瓦片集」
;; 支持 4 种地形（grass/dirt/stone/water）× 16 种过渡图案 = 64 瓦片
;;
;; 技术特性:
;;   - 自包含 SkSL 着色器（程序化噪声纹理 + FBM 域扭曲 + SDF + smoothstep）
;;   - 相邻双角用直线避免「掐腰」
;;   - 三角（concave）用反转 SDF
;;   - 每块瓦片不同噪声种子，域扭曲不同
;;
;; 输出:
;;   00_spritesheet.png — 64 瓦片总表
;;   01_demo_tilemap.png — dirt 地形过渡演示
;;   02_cross_terrain.png — 跨地形过渡演示
;; ═══════════════════════════════════════════════════════════════════════════════
(set-backend! "skia")
(println "testing shader...")
(define sksl (shader "
uniform float u_bl, u_br, u_tl, u_tr;
uniform float u_seed, u_type;

float hash(float2 p) {
    return fract(sin(dot(p, float2(127.1, 311.7))) * 43758.5453123);
}

float fbm3(float2 p) {
    float v=0.0;
    {float2 ip=floor(p);float2 fp=fract(p);fp=fp*fp*(3.0-2.0*fp);v+=0.5*mix(mix(hash(ip+float2(0,0)),hash(ip+float2(1,0)),fp.x),mix(hash(ip+float2(0,1)),hash(ip+float2(1,1)),fp.x),fp.y);}
    {p*=2.0;float2 ip=floor(p);float2 fp=fract(p);fp=fp*fp*(3.0-2.0*fp);v+=0.25*mix(mix(hash(ip+float2(0,0)),hash(ip+float2(1,0)),fp.x),mix(hash(ip+float2(0,1)),hash(ip+float2(1,1)),fp.x),fp.y);}
    {p*=2.0;float2 ip=floor(p);float2 fp=fract(p);fp=fp*fp*(3.0-2.0*fp);v+=0.125*mix(mix(hash(ip+float2(0,0)),hash(ip+float2(1,0)),fp.x),mix(hash(ip+float2(0,1)),hash(ip+float2(1,1)),fp.x),fp.y);}
    return v;
}

float fbm2(float2 p) {
    float v=0.0;
    {float2 ip=floor(p);float2 fp=fract(p);fp=fp*fp*(3.0-2.0*fp);v+=0.5*mix(mix(hash(ip+float2(0,0)),hash(ip+float2(1,0)),fp.x),mix(hash(ip+float2(0,1)),hash(ip+float2(1,1)),fp.x),fp.y);}
    {p*=2.0;float2 ip=floor(p);float2 fp=fract(p);fp=fp*fp*(3.0-2.0*fp);v+=0.25*mix(mix(hash(ip+float2(0,0)),hash(ip+float2(1,0)),fp.x),mix(hash(ip+float2(0,1)),hash(ip+float2(1,1)),fp.x),fp.y);}
    return v;
}

half3 terrain_color(float2 uv, float seed, float type) {
    float n=fbm2(uv*0.03+seed);
    float n2=fbm2(uv*0.06+seed+100.0);
    if (type<0.5) {
        float g=0.18+n*0.30+n2*0.08;
        return mix(half3(0.10,0.30,0.08), half3(0.25,0.65,0.20), g);
    } else if (type<1.5) {
        float d=0.30+n*0.25+n2*0.10;
        return mix(half3(0.30,0.18,0.08), half3(0.65,0.50,0.30), d);
    } else if (type<2.5) {
        float s=0.30+n*0.30+n2*0.15;
        return mix(half3(0.25,0.30,0.35), half3(0.55,0.58,0.62), s);
    } else {
        float w=0.20+n*0.25+n2*0.10;
        return mix(half3(0.05,0.15,0.40), half3(0.15,0.60,0.90), w);
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
    float alpha=1.0-smoothstep(-sw,sw,fill);
    return half4(mix(gcol,tcol,alpha), 1.0);
}
"))
(println "  compiled OK")
