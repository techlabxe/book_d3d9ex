struct VS_INPUT {
    float4 Pos: POSITION;
    float2 UV: TEXCOORD0;
};
struct VS_OUTPUT {
    float4 Pos : POSITION;
    float2 UV: TEXCOORD0;
};

matrix mtxWorld : register(c0);
matrix mtxViewProj : register(c4);

VS_OUTPUT main(VS_INPUT _In)
{
    VS_OUTPUT vsOut = (VS_OUTPUT)0;

    float4 worldPos = mul(_In.Pos, mtxWorld);
    vsOut.Pos = mul(worldPos, mtxViewProj);
    vsOut.UV = _In.UV;

    return vsOut;
}
