struct VertexInputType
{
    float4 Position : POSITION;
    float4 Color : COLOR;
};

struct PixelInputType
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
};

PixelInputType main(VertexInputType _Input)
{
    PixelInputType _Output;
    _Output.Position = _Input.Position;
    _Output.Color = _Input.Color;
    return _Output;
}