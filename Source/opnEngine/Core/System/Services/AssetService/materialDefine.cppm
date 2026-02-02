module;
#include <optional>
#include <string>

#include "hlsl++.h"

export module opn.Assets.materialDefine;
import opn.System.UUID;

export namespace opn {
    struct Material {
        std::string name;
        std::optional<UUID> albedoTexture;
        std::optional<UUID> normalTexture;
        hlslpp::float4 baseColor{ 1, 1, 1, 1 };
        float metallic;
        float roughness;
    };
}
