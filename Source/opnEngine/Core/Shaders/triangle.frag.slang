#include "vertexData.slang"

[ shader( "fragment" ) ]
float4 main( VertexOutput input ) : SV_TARGET
{
    return float4(input.colour,1.0f);
};