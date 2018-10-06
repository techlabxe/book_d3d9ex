struct VS_INPUT {
    float4 Pos: POSITION;
    float3 Normal : NORMAL;
};
struct VS_OUTPUT {
    float4 Pos : POSITION;
    float4 WorldPos : TEXCOORD0;
    float3 WorldNormal : TEXCOORD1;
    float4 Color : COLOR;
};

matrix mtxWorld : register(c0);
matrix mtxViewProj : register(c4);
float4 diffuse : register(c8);

VS_OUTPUT main(VS_INPUT _In)
{
    VS_OUTPUT vsOut = (VS_OUTPUT)0;

    float4 worldPos = mul(_In.Pos, mtxWorld);
    float3 worldNormal = mul(_In.Normal, (float3x3)mtxWorld);
    vsOut.Pos = mul(worldPos, mtxViewProj);
    vsOut.WorldPos = worldPos;
    vsOut.WorldNormal = worldNormal;
    vsOut.Color = diffuse;

    return vsOut;
}
