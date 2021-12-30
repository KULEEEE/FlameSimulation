

#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 1
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif


Texture2D    gDiffuseMap : register(t0);
Texture2D    gDiffuseMap2 : register(t1);


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

    float blur;
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

    vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);
     
    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    // Transform to homogeneous clip space.
    vout.PosH = mul(posW, gViewProj);
    vout.NormalW.x = vout.PosH.z/vout.PosH.w;
	// Output vertex attributes for interpolation across triangle.
	vout.TexC = mul(float4(vin.TexC, 0.0f, 1.0f), gMatTransform).xy;
    vout.Lifespan = vin.Lifespan;
    return vout;
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
    
    //float4 color = float4(2.0f, min(2.0f,max(pin.Lifespan*2 - 0.4f ,0.0f)*2.0f), min(1.0f, max(pin.Lifespan * 2 - 1.2f, 0.0f)*2.0f),pin.Lifespan)*diffuseAlbedo;

    float4 color = float4(1.0f, 0.6*pin.Lifespan, pin.NormalW.x, pin.Lifespan) * diffuseAlbedo;

    //float4 color = float4(2.0f,2.0f,2.0f,2.0f) * diffuseAlbedo;

    
    

    
    return float4(color.rgb, color.a*0.2);
    
    //return float4(0.5f,0.5f,0.5f,1.0f);
    
    
}

float4 PS2(VertexOut pin) : SV_Target
{
    
    float4 diffuseAlbedo = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC);
    //float4 diffuseAlbedo = float4(1.0f, 1.0f, 1.0f, 1.0f);
        
    float3 mapped = diffuseAlbedo / (diffuseAlbedo + float3(1.0f, 1.0f, 1.0f));
    
    

    pin.NormalW = normalize(pin.NormalW);
    
     
    float3 toEyeW = normalize(gEyePosW - pin.PosW);
    
    
    float4 ambient = gAmbientLight * gDiffuseAlbedo;
    
    const float shininess = 1.0f - gRoughness;
    Material mat = { float4(mapped,1.0f), gFresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW, pin.NormalW, toEyeW, shadowFactor);
    
    float4 litColor = directLight;
    
        
    litColor.a = diffuseAlbedo.a;

    return litColor;
    //return float4(mapped, 1.0f);
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
    float w1, w2, w3, w4, w5, w6, w7,w8;
    w1 = gDiffuseMap.Sample(gsamAnisotropicWrap,pin.TexC).a - gDiffuseMap.Sample(gsamAnisotropicWrap,pin.TexC+float2(-blur,blur)).a;
    w2 = gDiffuseMap.Sample(gsamAnisotropicWrap,pin.TexC).a - gDiffuseMap.Sample(gsamAnisotropicWrap,pin.TexC+float2( 0 ,blur)).a;
    w3 = gDiffuseMap.Sample(gsamAnisotropicWrap,pin.TexC).a - gDiffuseMap.Sample(gsamAnisotropicWrap,pin.TexC+float2( blur,blur)).a;
    w4 = gDiffuseMap.Sample(gsamAnisotropicWrap,pin.TexC).a - gDiffuseMap.Sample(gsamAnisotropicWrap,pin.TexC+float2( blur,0)).a;
    w5 = gDiffuseMap.Sample(gsamAnisotropicWrap,pin.TexC).a - gDiffuseMap.Sample(gsamAnisotropicWrap,pin.TexC+float2( blur,-blur)).a;
    w6 = gDiffuseMap.Sample(gsamAnisotropicWrap,pin.TexC).a - gDiffuseMap.Sample(gsamAnisotropicWrap,pin.TexC+float2( 0,-blur)).a;
    w7 = gDiffuseMap.Sample(gsamAnisotropicWrap,pin.TexC).a - gDiffuseMap.Sample(gsamAnisotropicWrap,pin.TexC+float2(-blur,-blur)).a;
    w8 = gDiffuseMap.Sample(gsamAnisotropicWrap,pin.TexC).a - gDiffuseMap.Sample(gsamAnisotropicWrap,pin.TexC+float2(-blur,0)).a;

    return float4(-w1+w3+w4+w5-w7-w8,w1+w2+w3-w5-w6-w7,0.0f,1.0f);
    //return float4(+w1-w3-w4-w5+w7+w8,w1-w2-w3+w5+w6+w7,0.0f,1.0f);
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
    float distance = sqrt(dot((gEyePosW - ginitialPoint),(gEyePosW - ginitialPoint)));
    float3 dir = normalize(gDiffuseMap2.Sample(gsamAnisotropicWrap, pin.TexC).rgb);
    float4 color;
    float3 du;
    //color = float4(normalize(gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC).rgb), 1.1 / (1 + pow(exp, (-gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC).a + 0.5f))) - 0.1f);
    //color = float4(1.1 / (1 + pow(exp, (-gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC).r + 0.5f))) - 0.1f, 1.1 / (1 + pow(exp, (-gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC).g + 0.5f))) - 0.1f, 1.1 / (1 + pow(exp, (-gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC).b + 0.5f))) - 0.1f, 1.1 / (1 + pow(exp, (-gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC).a + 0.5f))) - 0.1f);
    //color = float4(gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC).r/ gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC).r, gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC).g/ gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC).r, gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC).b / gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC).r, 1.1 / (1 + pow(exp, (-gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC).a + 0.5f))) - 0.1f);

        
    du = dir/distance*0.09;
    
    if((pin.TexC+du).x>=0&&(pin.TexC+du).y>=0&&(pin.TexC+du).x<=1&&(pin.TexC+du).y<=1)
    {
        color = float4((gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC + float2(du.x, du.y)).rgb +gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC).rgb),clamp(gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC).a*gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC+ float2(du.x, du.y)).a,0.0f,1.0f));
    }
    else
    {
        color = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC);
    }
    //color = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC);
    color.b= 0;
    //color.g = clamp(color.g,0.0f,2.0f)*0.5f;
    //color.g= (color.g / (color.g+ 1.0f)*0.7);
    color.g = (color.g / (color.g+ 1.0f)*0.8)*0.5f + clamp(color.g,0.0f,2.0f)*0.25f;

    color.a = clamp(color.a,0.0f,0.9f);
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


