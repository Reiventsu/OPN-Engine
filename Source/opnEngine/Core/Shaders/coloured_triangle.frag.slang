[shader("fragment")]
float4 main(
    float3 inColor : COLOR0
) : SV_Target {
    return float4(inColor, 1.0);
}