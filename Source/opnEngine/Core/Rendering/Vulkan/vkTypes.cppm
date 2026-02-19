module;

////////////////////////////////////////////////////
//
//  Based on the vk_types.h from vkguide but
//  modularized and converted to work with my stuff
//
////////////////////////////////////////////////////

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>
#include <iostream>
#include <source_location>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "hlsl++.h"

export module opn.Rendering.Util.vk.vkTypes;

export namespace opn::vkUtil {



    struct sAllocatedImage {
        VkImage       image;
        VkImageView   imageView;
        VmaAllocation allocation;
        VkExtent3D    imageExtent;
        VkFormat      imageFormat;
    };

    struct sAllocatedBuffer {
        VkBuffer          buffer;
        VmaAllocation     allocation;
        VmaAllocationInfo info;
    };

    struct sGPUGLTFMaterial {
        hlslpp::float4 colorFactor;
        hlslpp::float4 metallicRoughnessFactor;
        hlslpp::float4 extra[ 14 ];
    };

    static_assert( sizeof( sGPUGLTFMaterial ) == 256 );

    struct sGPUSceneData {
        hlslpp::float4x4 viewMatrix;
        hlslpp::float4x4 projectionMatrix;
        hlslpp::float4x4 viewProjectionMatrix;
        hlslpp::float4   ambientColor;
        hlslpp::float4   sunlightDirection; // w for sun power
        hlslpp::float4   sunlightColor;
    };

    struct sGPUMeshStreams {
        sAllocatedBuffer positions;
        sAllocatedBuffer normals;
        sAllocatedBuffer uvs;
        sAllocatedBuffer indices;
    };

    enum class sMaterialPass : uint8_t {
        MainColor,
        Transparent,
        Other
    };

    struct sMaterialPipeline {
        VkPipeline       pipeline;
        VkPipelineLayout layout;
    };

    struct sMaterialInstance {
        sMaterialPipeline *pipeline;
        VkDescriptorSet   materialSet;
        sMaterialPass      passType;
    };

    struct sVertex {
        hlslpp::float3 position;
        hlslpp::float3 normal;
        hlslpp::float4 color;
        hlslpp::float2 uv;
    };

    // holds the resources needed for a mesh
    struct sGPUMeshBuffers {
        sAllocatedBuffer indexBuffer;
        sAllocatedBuffer vertexBuffer;
        VkDeviceAddress vertexBufferAddress;
    };

    // push constants for our mesh object draws
    struct sGPUDrawPushConstants {
        hlslpp::float4x4 worldMatrix;
        VkDeviceAddress  vertexBuffer;
    };

    struct sDrawContext;

    // base class for a renderable dynamic object
    class iRenderable {
    public:
        virtual ~iRenderable() = default;
        virtual void Draw(const hlslpp::float4x4 &topMatrix, sDrawContext &ctx) = 0;
    };

    // implementation of a drawable scene node.
    // the scene node can hold children and will also keep a transform to propagate
    // to them
    struct sNode : public iRenderable {
        // parent pointer must be a weak pointer to avoid circular dependencies
        std::weak_ptr<sNode> parent;
        std::vector<std::shared_ptr<sNode>> children;

        hlslpp::float4x4 localTransform;
        hlslpp::float4x4 worldTransform;

        void refreshTransform(const hlslpp::float4x4 &parentMatrix) {
            worldTransform = parentMatrix * localTransform;
            for (auto c : children) {
                c->refreshTransform(worldTransform);
            }
        }

        void Draw(const hlslpp::float4x4 &topMatrix, sDrawContext &ctx) override {
            // draw children
            for (auto &c : children) {
                c->Draw(topMatrix, ctx);
            }
        }
    };

    // Vulkan error checking context
    struct sVkContext {
        const char *operation;
        std::source_location location;

        sVkContext(const char *_operation,
                  const std::source_location _location = std::source_location::current())
            : operation(_operation), location(_location) {}
    };

    inline void vkCheck(VkResult _result, sVkContext _ctx) {
        if (_result != VK_SUCCESS) {
            std::string_view filename = _ctx.location.file_name();
            if (const auto pos = filename.find_last_of("/\\"); pos != std::string_view::npos) {
                filename = filename.substr(pos + 1);
            }

            std::println(std::cerr, "[VULKAN ERROR] ( {}:{} ) - {} failed",
                         filename, _ctx.location.line(), _ctx.operation);
            std::abort();
        }
    }
}