module;
#include "volk.h"
export module opn.ECS.Components:ShaderOverride;

export namespace opn::components {
    struct ShaderOverride {
        VkPipeline       specialPipeline       = VK_NULL_HANDLE;
        VkPipelineLayout specialPipelineLayout = VK_NULL_HANDLE;
    };
}