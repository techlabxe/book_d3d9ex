struct VS_OUTPUT {
    float4 Pos : POSITION;
    float4 WorldPos : TEXCOORD0;
    float3 WorldNormal : TEXCOORD1;
    float4 Color : COLOR;
};

struct PS_OUTPUT
{
    float4 WorldPos : COLOR0;
    float4 WorldNormal : COLOR1;
    float4 Diffuse : COLOR2;
};

PS_OUTPUT main(VS_OUTPUT _In)
{
    PS_OUTPUT psOut = (PS_OUTPUT)0;
    psOut.WorldPos = _In.WorldPos;
    psOut.WorldNormal = float4(_In.WorldNormal,1);
    psOut.Diffuse = _In.Color;

    return psOut;
}