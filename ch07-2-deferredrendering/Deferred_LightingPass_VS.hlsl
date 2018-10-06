struct VS_INPUT
{
    float4 Pos : POSITION;
    float2 UV : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 Pos : POSITION;
    float2 UV : TEXCOORD0;
};

//#@@range_begin(vertex_offset)
VS_OUTPUT main(VS_INPUT _In)
{
    VS_OUTPUT vsOut = (VS_OUTPUT)0;
    
    vsOut.Pos = _In.Pos;
    vsOut.Pos.x -= 0.5 / 1280;
    vsOut.Pos.y += 0.5 / 720;
    vsOut.UV = _In.UV;
    return vsOut;
}
//#@@range_end(vertex_offset)
