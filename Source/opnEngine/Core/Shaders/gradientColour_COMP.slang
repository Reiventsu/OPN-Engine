struct sPushConstants {
    float4 data1;
    float4 data2;
    float4 data3;
    float4 data5;
} pushConstants;

[ shader( "compute" ) ]
[numthreads( 16, 16, 1 ) ]
void main(
    uint3 _dispatchThreadID: SV_DispatchThreadID,
    uniform RWTexture2D<float4> _image
)
{
    int2 texelCoord = int2( _dispatchThreadID.xy );

    uint width, height;
    _image.GetDimensions( width, height );
    int2 size = int2( width, height );

    float4 topColour = pushConstants.data1;
    float4 bottomColour = pushConstants.data2;

    if (texelCoord.x < size.x && texelCoord.y < size.y) 
    {
        float blend = float(texelCoord.y) / float(size.y);

        float4 colour = lerp(topColour, bottomColour, blend);
        image[texelCoord] = colour;
    }
}