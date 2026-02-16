module;
#include "volk.h"
#include <vector>
#include <cstdint>

export module opn.Renderer.Types;

export namespace opn {
    struct sSetLayoutData {
        uint32_t setIndex;
        std::vector<VkDescriptorSetLayoutBinding> bindings;
    };

    struct sShaderReflection {
        std::vector<uint32_t> byteCode; 
        std::vector<sSetLayoutData> setLayouts;
        std::vector<VkPushConstantRange> pushConstants;
    };
}