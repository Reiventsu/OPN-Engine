module;
#include "hlsl++.h"
export module opn.ECS.Components.Transform;

export namespace opn::components {
    struct Transform {
        hlslpp::float3 position{ 0.0f, 0.0f, 0.0f };
        hlslpp::quaternion rotation{ 0.0f, 0.0f, 0.0f, 1.0f };
        hlslpp::float3 scale{ 1.0f, 1.0f, 1.0f };

        hlslpp::float4x4 getMatrix() const {
            auto T = hlslpp::float4x4::translation(position);
            auto R = hlslpp::float4x4::rotation_axis(rotation.xyz, rotation.w);
            auto S = hlslpp::float4x4::scale(scale);
            return T * R * S;
        };
    };
}