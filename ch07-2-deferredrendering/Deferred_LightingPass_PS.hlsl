struct VS_OUTPUT
{
    float4 Pos : POSITION;
    float2 UV : TEXCOORD0;
};

sampler2D texWorldPos : register(s0);
sampler2D texWorldNormal : register(s1);
sampler2D texDiffuse : register(s2);

#define NUM_LIGHTS (16)

struct LightInfo
{
    float4 PosAndRadius;
    float4 Color;
};

LightInfo lightInfo[NUM_LIGHTS] : register(c0);

//#@@range_begin(Attenuation)
float Attenuation(float lightRadius, float distance)
{
    return 1.0 - smoothstep(lightRadius * 0.6, lightRadius, distance);
}
//#@@range_end(Attenuation)

float4 main(VS_OUTPUT _In) : COLOR
{
    float2 uv = _In.UV.xy;
    float4 color = float4(0,0,0,1);

    float4 diffuse = tex2D(texDiffuse, uv);
    float4 world = tex2D(texWorldPos, uv);
    float4 worldNormal = normalize(tex2D(texWorldNormal, uv));

//#@@range_begin(lighting)
    for (int i = 0; i < NUM_LIGHTS; ++i)
    {
        LightInfo light = lightInfo[i];
        float3 lpos = light.PosAndRadius.xyz;
        float lr = light.PosAndRadius.w;


        float3 L = lpos - world.xyz;
        float3 lightDir = normalize(L);
        float att = Attenuation(lr, length(L));

        float3 lighting = max(0, dot(lightDir, worldNormal.xyz));
        lighting *= light.Color.xyz * att;

        color.xyz += lighting * diffuse.xyz;
    }
    return color;
//#@@range_end(lighting)
}