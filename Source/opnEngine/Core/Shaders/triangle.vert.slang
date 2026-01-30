#include "vertexData.slang"

[shader("vertex")]
VertexOutput main(uint vertexID: SV_VertexID) {
  const float3 positions[3] = { float3(1.0f, 1.0f, 0.0f),
                                float3(-1.0f, 1.0f, 0.0f),
                                float3(0.0f, -1.0f, 0.0f) };

  const float3 colours[3] = { float3(1.0f, 0.0f, 0.0f),
                              float3(0.0f, 1.0f, 0.0f),
                              float3(0.0f, 0.0f, 1.0f) };

  VertexOutput output;
  output.position = float4(positions[vertexID], 1.0f);
  output.colour = colours[vertexID];

  return output;
};
