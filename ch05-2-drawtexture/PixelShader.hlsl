struct VS_OUTPUT {
    float4 Pos : POSITION;
    float2 UV: TEXCOORD0;
};

sampler2D tex0 : register(s0);

float4 main(VS_OUTPUT _In) : COLOR
{
    float4 color = tex2D(tex0, _In.UV.xy);
    return color;
}