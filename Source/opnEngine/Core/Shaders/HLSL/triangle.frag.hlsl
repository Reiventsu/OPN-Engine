struct PSInput {
    float4 position : SV_POSITION;
    float3 color : COLOR0;
};

struct PSOutput {
    float4 color : SV_TARGET0;
};

PSOutput main(PSInput input) {
    PSOutput output;
    output.color = float4(input.color, 1.0);
    return output;
}