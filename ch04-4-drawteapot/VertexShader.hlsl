struct VS_INPUT {
    float4 Pos: POSITION;
    float3 Normal: NORMAL;
};
struct VS_OUTPUT {
    float4 Pos : POSITION;
    float4 Color : COLOR;
};

matrix mtxWorld : register(c0);
matrix mtxViewProj : register(c4);

VS_OUTPUT main(VS_INPUT _In)
{
    VS_OUTPUT vsOut = (VS_OUTPUT)0;
    float3 lightDir = normalize(float3(0.0,1.0,-1.0));


    float4 worldPos = mul(_In.Pos, mtxWorld);
    float3 worldNormal = mul(_In.Normal, (float3x3)mtxWorld);
    worldNormal = normalize(worldNormal);

    // Lighiting.
    float l = saturate(dot(worldNormal, lightDir));

    vsOut.Pos = mul(worldPos, mtxViewProj);
    vsOut.Color = float4(l, l, l, 1);
    return vsOut;
}
