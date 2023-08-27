// #include "VertexShader.hlsl"

struct PixelInputType
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
};


float4 main(PixelInputType _Input) : SV_TARGET
{
    return _Input.Color;
}