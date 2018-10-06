struct VS_INPUT {
    float4 Pos: POSITION;
    float4 Color: COLOR;
};
struct VS_OUTPUT {
    float4 Pos : POSITION;
    float4 Color : COLOR;
};

matrix mtxWVP : register(c0);

VS_OUTPUT main(VS_INPUT _In)
{
    VS_OUTPUT vsOut = (VS_OUTPUT)0;
    
    vsOut.Pos = mul(_In.Pos, mtxWVP);
    vsOut.Color = _In.Color;

    return vsOut;
}
