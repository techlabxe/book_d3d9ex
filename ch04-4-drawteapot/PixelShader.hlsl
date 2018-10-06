struct VS_OUTPUT {
    float4 Pos : POSITION;
    float4 Color : COLOR;
};


float4 main(VS_OUTPUT _In) : COLOR
{
    return _In.Color;
}