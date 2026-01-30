struct Vertex {
    float3 position;
    float  _pad0;
    float3 normal;
    float  _pad1;
    float4 color;
    float2 uv;
    float2 _pad2;
};

struct PushConstants {
    float4x4 worldMatrix;
    Vertex* vertices;
};

[[vk::push_constant]]
PushConstants pc;

struct V2F {
    float4 pos   : SV_Position;
    float3 color : COLOR0;
};

[shader("vertex")]
V2F main(uint vIdx : SV_VertexID) {
    V2F output;

    Vertex v = pc.vertices[vIdx];

    output.pos = mul(pc.worldMatrix, float4(v.position, 1.0));
    output.color = v.color.xyz;
    return output;
}