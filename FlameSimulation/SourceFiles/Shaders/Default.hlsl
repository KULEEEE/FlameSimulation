

#ifndef NUM_DIR_LIGHTS
    // No directional sun in this scene — the only illumination comes from
    // the flame itself + ambient. Lighting equation skips the directional loop.
    #define NUM_DIR_LIGHTS 0
#endif

#ifndef NUM_POINT_LIGHTS
    // Two warm point lights placed at the flame source. They illuminate the
    // terrain and the wood/log boxes underneath the fire.
    #define NUM_POINT_LIGHTS 2
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif


Texture2D    gDiffuseMap  : register(t0);
Texture2D    gDiffuseMap2 : register(t1);
Texture2D    gBloomMap    : register(t2);


SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);


#include "LightingUtil.hlsl"
// Constant data that varies per frame.
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
};

// Constant data that varies per material.
cbuffer cbPass : register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    float4 gAmbientLight;

    Light gLights[MaxLights];
    float3 ginitialPoint;

    float  blur;
    float  gBloomThreshold;
    float  gBloomIntensity;
    float2 gBloomPad;
};

cbuffer cbMaterial : register(b2)
{
	float4   gDiffuseAlbedo;
    float3   gFresnelR0;
    float    gRoughness;
	float4x4 gMatTransform;
};




struct VertexIn
{
	float3 PosL    : POSITION;
	float2 TexC    : TEXCOORD;
    float Lifespan : LIFESPAN;
    float3 NormalL  : NORMAL;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
	float2 TexC    : TEXCOORD;
    float Lifespan : LIFESPAN;
    float3 NormalW : NORMAL;
};


VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

    // Transform to world space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    // Proper world-space normal — used by PS2 for terrain lighting.
    // (This .x channel was previously being clobbered with clip-space depth,
    // silently breaking the lit-terrain shader; the depth was only ever
    // consumed by PS as a colour fudge for the "blue" channel — obsolete now
    // that PS uses a proper blackbody ramp.)
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);

    vout.PosH = mul(posW, gViewProj);
    vout.TexC = mul(float4(vin.TexC, 0.0f, 1.0f), gMatTransform).xy;
    vout.Lifespan = vin.Lifespan;
    return vout;
}

//---------------------------------------------------------------------------------------
// ACES Filmic tone mapping (Narkowicz approximation).
// Compresses HDR values > 1 with a smooth highlight rolloff while preserving
// mid-tone colour. Used at the end of PS_map to absorb the additive blowout
// from densely overlapped particles instead of letting the hot core clip
// channel-by-channel into pinkish-white.
//---------------------------------------------------------------------------------------
float3 ACESFilm(float3 x)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

//---------------------------------------------------------------------------------------
// Stylised blackbody flame colour ramp.
// Maps particle age (1.0 = freshly emitted / hottest, 0.0 = dying / coolest)
// to a 5-stop warm gradient. Two deliberate non-physical compromises:
//   - The "ember" end stays as a dim glowing red instead of fading to black.
//     Additive accumulation of near-black low-life particles otherwise reads
//     as muddy brown, not as a smouldering red ember.
//   - The "hot" end caps at warm yellow rather than bluish white. Pre-tone-map
//     LDR additive blend already saturates densely-overlapped pixels to white;
//     keeping B low at the source delays that clip and biases the spill warm.
//---------------------------------------------------------------------------------------
float3 FlameColor(float life)
{
    // Brightness-graded ramp.
    //
    // Real fire follows Stefan-Boltzmann: radiant intensity scales roughly
    // with T^4, so a 2× temperature increase means ~16× brightness. The cool
    // (red) end therefore must be MUCH dimmer than the hot (yellow) end —
    // not just a different hue at the same brightness.
    //
    // Without this gradient the additive accumulation flattens everything to
    // a uniform yellow because cool particles contribute almost as much
    // luminance as hot ones, and ACES drives the sum toward (1, 1, ~) for
    // any sufficient overlap.
    //
    // Hot end is deliberately pushed >1.0 (HDR) so the core blows out into a
    // smooth bright white-yellow under ACES, while cool stops stay dim
    // enough that even heavy overlap reads as red, not orange.
    const float3 cEmber  = float3(0.18f, 0.01f, 0.00f);  // very dim, near-black red
    const float3 cRed    = float3(0.55f, 0.06f, 0.00f);  // dim glowing red
    const float3 cOrange = float3(0.95f, 0.28f, 0.00f);  // saturated orange
    const float3 cYellow = float3(1.20f, 0.50f, 0.04f);  // bright orange-yellow (HDR)
    const float3 cHot    = float3(1.60f, 0.95f, 0.30f);  // hot core, well into HDR

    float3 col = lerp(cEmber,  cRed,    smoothstep(0.0f,  0.25f, life));
    col        = lerp(col,     cOrange, smoothstep(0.25f, 0.5f,  life));
    col        = lerp(col,     cYellow, smoothstep(0.5f,  0.75f, life));
    col        = lerp(col,     cHot,    smoothstep(0.75f, 1.0f,  life));
    return col;
}

float4 PS(VertexOut pin) : SV_Target
{
    int temp = 50;
    float width = 2048;
    float height = 2048;

    //float4 diffuseAlbedo = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC);

    float4 diffuseAlbedo = (gDiffuseMap.Sample(gsamAnisotropicWrap, float2(pin.TexC.x + temp / width, pin.TexC.y + temp / height))
        + gDiffuseMap.Sample(gsamAnisotropicWrap, float2(pin.TexC.x, pin.TexC.y + temp / height))
        + gDiffuseMap.Sample(gsamAnisotropicWrap, float2(pin.TexC.x + temp / width, pin.TexC.y))
        + gDiffuseMap.Sample(gsamAnisotropicWrap, float2(pin.TexC.x, pin.TexC.y))
        + gDiffuseMap.Sample(gsamAnisotropicWrap, float2(pin.TexC.x - temp / width, pin.TexC.y - temp / height))
        + gDiffuseMap.Sample(gsamAnisotropicWrap, float2(pin.TexC.x, pin.TexC.y - temp / height))
        + gDiffuseMap.Sample(gsamAnisotropicWrap, float2(pin.TexC.x - temp / width, pin.TexC.y))
        + gDiffuseMap.Sample(gsamAnisotropicWrap, float2(pin.TexC.x - temp / width, pin.TexC.y + temp / height))
        + gDiffuseMap.Sample(gsamAnisotropicWrap, float2(pin.TexC.x + temp / width, pin.TexC.y - temp / height))) / 9.0f;
    
    // Drive emission colour from the blackbody-style ramp keyed on particle age.
    // diffuseAlbedo carries the soft-blob alpha mask from the particle texture.
    float3 emissive = FlameColor(pin.Lifespan);
    float4 color    = float4(emissive * diffuseAlbedo.rgb, pin.Lifespan * diffuseAlbedo.a);

    return float4(color.rgb, color.a * 0.2f);
}

float4 PS2(VertexOut pin) : SV_Target
{
    float4 diffuseAlbedo = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC);

    pin.NormalW = normalize(pin.NormalW);
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

    // Standard ambient + direct lighting in linear HDR space. (The previous
    // code applied Reinhard to the *albedo* before lighting, which crushed
    // texture contrast without doing any actual highlight rolloff. Tone map
    // belongs at the very end, on the lit colour.)
    float4 ambient = gAmbientLight * diffuseAlbedo;

    const float shininess = 1.0f - gRoughness;
    Material mat = { diffuseAlbedo, gFresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW, pin.NormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

    // ACES tonemap so close-range point-light spill rolls off smoothly into
    // warm white instead of clipping into pure yellow on the terrain.
    litColor.rgb = ACESFilm(litColor.rgb);

    litColor.a = diffuseAlbedo.a;
    return litColor;
}

VertexOut VS_tmap(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;
    vout.PosH = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.TexC = mul(float4(vin.TexC, 0.0f, 1.0f), gMatTransform).xy;
    //vout.PosH = mul(posW, gViewProj);
    //vout.NormalW = mul(mul(vin.NormalL, (float3x3)gWorld),(float3x3)gViewProj);
    //vout.PosH = float4(vout.PosH.xy,0.0f,vout.PosH.w);
    return vout;
}

float4 PS_tmap(VertexOut pin) : SV_Target
{
    // Cache the center alpha once instead of resampling it for every w_n.
    float c  = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC).a;
    float a1 = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC + float2(-blur,  blur)).a;
    float a2 = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC + float2( 0.0f,  blur)).a;
    float a3 = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC + float2( blur,  blur)).a;
    float a4 = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC + float2( blur,  0.0f)).a;
    float a5 = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC + float2( blur, -blur)).a;
    float a6 = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC + float2( 0.0f, -blur)).a;
    float a7 = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC + float2(-blur, -blur)).a;
    float a8 = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC + float2(-blur,  0.0f)).a;

    float w1 = c - a1, w2 = c - a2, w3 = c - a3, w4 = c - a4;
    float w5 = c - a5, w6 = c - a6, w7 = c - a7, w8 = c - a8;

    // Compass-style gradient of alpha: gx ≈ ∂α/∂x, gy ≈ ∂α/∂y.
    float gx = -w1 + w3 + w4 + w5 - w7 - w8;
    float gy =  w1 + w2 + w3 - w5 - w6 - w7;

    // Rotate the gradient by 90° to produce a divergence-free (curl) field
    // that runs *along* iso-alpha contours instead of across them. The
    // downstream PS_map normalises this vector, so only the direction
    // matters — magnitude is irrelevant.
    //
    //     gradient (curl-free)        rotated (divergence-free)
    //         ↑   ↑   ↑                  →   →   →
    //      ↗  ↑  ↑  ↑  ↖              ↗   ↑   ↑   ↖
    //     →   ●   ←     etc.         ↑   curl   ↓
    //
    return float4(-gy, gx, 0.0f, 1.0f);
}

VertexOut VS_map(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;
    float4 posxy = mul(float4(vin.PosL, 1.0f), gWorld);
    float4 posh = mul(posxy, gViewProj);
    float z = posh.z/posh.w;
    vout.PosH = float4(posxy.xy,z,1.0f);
	vout.TexC = mul(float4(vin.TexC, 0.0f, 1.0f), gMatTransform).xy;
    vout.Lifespan = vin.Lifespan;
    return vout;
}

//---------------------------------------------------------------------------------------
// Multi-octave bloom — 3 shader stages:
//
//   1) PS_bloom            (extract): ParticleMap → BloomMip0
//        bright-extract + small Gaussian blur, only run once at the start.
//
//   2) PS_bloomDownsample            : BloomMip[K] → BloomMip[K+1]
//        progressive box downsample (relies on hardware bilinear, which
//        averages a 2×2 source neighbourhood for free when sampled at the
//        midpoint between source texels).
//
//   3) PS_bloomUpsample              : BloomMip[K+1] → BloomMip[K]  (ADDITIVE)
//        bilinear upsample, additively blended onto the larger mip's
//        existing downsample data. Repeated up the chain, this accumulates
//        all octaves of blur into mip 0, which the composite shader then
//        samples once.
//
// Compared to single-octave bloom the visible difference is range:
//     mip 0 keeps the tight halo close to bright pixels,
//     mip 5 (16² for an 800×600 backbuffer) covers most of the screen with
//          a soft, low-frequency glow.
//---------------------------------------------------------------------------------------
float4 PS_bloom(VertexOut pin) : SV_Target
{
    const float w[5] = { 0.0625f, 0.25f, 0.375f, 0.25f, 0.0625f };
    const float texelSize   = 1.0f / 2048.0f;   // ParticleMap is 2048²
    const float bloomSpread = 6.0f;              // texel jump per kernel step
    // gBloomThreshold comes from cbPass — tunable via ImGui.

    // 5×5 weighted Gaussian sample around the current UV.
    float3 sum = 0.0f;
    [unroll] for (int dy = -2; dy <= 2; ++dy)
    {
        [unroll] for (int dx = -2; dx <= 2; ++dx)
        {
            float  weight = w[dx + 2] * w[dy + 2];
            float2 offset = float2(dx, dy) * bloomSpread * texelSize;
            sum += gDiffuseMap.Sample(gsamLinearClamp, pin.TexC + offset).rgb * weight;
        }
    }

    // Bright-pass extract: only HDR-overshooting regions contribute, so cool
    // areas don't pick up a generic global glow.
    float3 bright = max(sum - gBloomThreshold, 0.0f);
    return float4(bright, 1.0f);
}

float4 PS_bloomDownsample(VertexOut pin) : SV_Target
{
    // Hardware bilinear at the destination UV samples the midpoint of a 2×2
    // neighbourhood in the (twice-as-large) source mip — that's a free 2×2
    // box average. Cheaper and visually equivalent here to an explicit 4-tap
    // filter.
    return float4(gDiffuseMap.Sample(gsamLinearClamp, pin.TexC).rgb, 1.0f);
}

float4 PS_bloomUpsample(VertexOut pin) : SV_Target
{
    // Sampled with bilinear from a smaller source mip; the destination mip's
    // PSO is configured with ADDITIVE blend, so this contribution is added
    // onto whatever the larger mip already holds (its own downsample data).
    return float4(gDiffuseMap.Sample(gsamLinearClamp, pin.TexC).rgb, 1.0f);
}

float4 PS_map(VertexOut pin) :SV_Target
{
    float exp = 100.0f;
    //return float4(gDiffuseMap2.Sample(gsamAnisotropicWrap,pin.TexC).rgb,1.0f);
    //return gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC);
    //return float4(pin.Lifespan,0.0f,0.0f,1.0f);
    //float alpha;w
    //return float4(gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC).rgb, 1.0f);
    //alpha = max(1 /(1 + pow(exp, (-gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC).a + 0.5f))) - 0.1f,0.0f);
    //return float4(1-gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC).a,0.0f,0.0f,1.0f);
    //return float4(gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC).rgb, pow(gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC).a /(gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC).a + 0.1f),2.2f));
    float distance = sqrt(dot((gEyePosW - ginitialPoint), (gEyePosW - ginitialPoint)));
    float3 dir = normalize(gDiffuseMap2.Sample(gsamAnisotropicWrap, pin.TexC).rgb);
    float4 color;

    // 2D advection offset (turbulence vector field is encoded in RG; B is
    // unused). Declaring as float2 avoids implicit-truncation warnings when
    // adding to pin.TexC below.
    float2 du = dir.xy / distance * 0.09f;
    float2 advectedUV = pin.TexC + du;

    // Sample the particle map at the current UV and at the curl-advected UV,
    // then *average* (not sum) them. The original code summed, which doubled
    // the brightness of every pixel along the advection path and pushed the
    // tonemap aggressively toward yellow saturation. Averaging preserves the
    // intended brightness profile from FlameColor while still letting the
    // turbulence map advect the texture colour.
    float4 c0 = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC);
    if (advectedUV.x >= 0.0f && advectedUV.y >= 0.0f &&
        advectedUV.x <= 1.0f && advectedUV.y <= 1.0f)
    {
        float4 c1 = gDiffuseMap.Sample(gsamAnisotropicWrap, advectedUV);
        color.rgb = (c0.rgb + c1.rgb) * 0.5f;
        color.a   = saturate(c0.a * c1.a);
    }
    else
    {
        color = c0;
    }

    // Add the precomputed multi-octave bloom (rendered into BloomMip 0 by
    // DrawBloomChain). gBloomIntensity is tunable from ImGui — the upsample
    // chain accumulates contributions from every mip, so values around 0.3
    // already give a strong halo; higher values push toward white-out.
    float3 bloom = gBloomMap.Sample(gsamLinearClamp, pin.TexC).rgb;
    color.rgb += bloom * gBloomIntensity;

    // Lift alpha where bloom is present so the halo composites onto the
    // terrain (mMapPSO uses SrcAlpha + InvSrcAlpha). The alpha lift scales
    // with intensity so cranking intensity also widens the visible halo,
    // and dropping intensity correspondingly lets terrain show through.
    float bloomLum = max(bloom.r, max(bloom.g, bloom.b));
    color.a = max(color.a, saturate(bloomLum * gBloomIntensity));

    // Tonemap the (now bloomed) HDR result so the hot core rolls off smoothly
    // into a warm white instead of channel-clipping. ACES compresses all
    // three channels coherently, preserving hue.
    color.rgb = ACESFilm(color.rgb);

    color.a = clamp(color.a, 0.0f, 0.95f);
    // blur shader
    
    //color.rgb = color.rgb/(color.rgb+float3(1,1,1));

    //color = float4(1.5f / (1 + pow(4.0f, (-color.r + 0.5f))) - 0.5f, 1.5f / (1 + pow(4.0f, (-color.g + 0.5f))) - 0.5f, 1.5f / (1 + pow(4.0f, (-color.b + 0.5f))) - 0.5f,1.1f / (1 + pow(exp, (-gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC).a + 0.5f))) - 0.1f); 
    
    return color;
}

VertexOut VS_boundary(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosH = mul(posW, gViewProj);
    //vout.PosH = float4(vout.PosH.xy,0.0f,vout.PosH.w);
    return vout;
}

float4 PS_boundary(VertexOut pin) : SV_Target
{
    return float4(1,0,0,1);
}


