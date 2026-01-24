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
#include <vulkan/vk_enum_string_helper.h>
#include <vk_mem_alloc.h>

#include "hlsl++.h"

export module opn.Rendering.Util.vk.vkTypes;

export namespace vkUtil {
    struct AllocatedImage {
        VkImage image;
        VkImageView imageView;
        VmaAllocation allocation;
        VkExtent3D imageExtent;
        VkFormat imageFormat;
    };

    struct AllocatedBuffer {
        VkBuffer buffer;
        VmaAllocation allocation;
        VmaAllocationInfo info;
    };

    struct GPUGLTFMaterial {
        hlslpp::float4 colorFactor;
        hlslpp::float4 metallicRoughnessFactor;
        hlslpp::float4 extra[14];
    };

    static_assert(sizeof(GPUGLTFMaterial) == 256);

    struct GPUSceneData {
        hlslpp::float4x4 viewMatrix;
        hlslpp::float4x4 projectionMatrix;
        hlslpp::float4x4 viewProjectionMatrix;
        hlslpp::float4 ambientColor;
        hlslpp::float4 sunlightDirection; // w for sun power
        hlslpp::float4 sunlightColor;
    };

    enum class MaterialPass : uint8_t {
        MainColor,
        Transparent,
        Other
    };

    struct MaterialPipeline {
        VkPipeline pipeline;
        VkPipelineLayout layout;
    };

    struct MaterialInstance {
        MaterialPipeline *pipeline;
        VkDescriptorSet materialSet;
        MaterialPass passType;
    };

    struct Vertex {
        hlslpp::float3 position;
        float uv_x;
        hlslpp::float3 normal;
        float uv_y;
        hlslpp::float4 color;
    };

    // holds the resources needed for a mesh
    struct GPUMeshBuffers {
        AllocatedBuffer indexBuffer;
        AllocatedBuffer vertexBuffer;
        VkDeviceAddress vertexBufferAddress;
    };

    // push constants for our mesh object draws
    struct GPUDrawPushConstants {
        hlslpp::float4x4 worldMatrix;
        VkDeviceAddress vertexBuffer;
    };

    struct DrawContext;

    // base class for a renderable dynamic object
    class IRenderable {
    public:
        virtual ~IRenderable() = default;
        virtual void Draw(const hlslpp::float4x4 &topMatrix, DrawContext &ctx) = 0;
    };

    // implementation of a drawable scene node.
    // the scene node can hold children and will also keep a transform to propagate
    // to them
    struct Node : public IRenderable {
        // parent pointer must be a weak pointer to avoid circular dependencies
        std::weak_ptr<Node> parent;
        std::vector<std::shared_ptr<Node>> children;

        hlslpp::float4x4 localTransform;
        hlslpp::float4x4 worldTransform;

        void refreshTransform(const hlslpp::float4x4 &parentMatrix) {
            worldTransform = parentMatrix * localTransform;
            for (auto c : children) {
                c->refreshTransform(worldTransform);
            }
        }

        void Draw(const hlslpp::float4x4 &topMatrix, DrawContext &ctx) override {
            // draw children
            for (auto &c : children) {
                c->Draw(topMatrix, ctx);
            }
        }
    };

    // Vulkan error checking context
    struct VkContext {
        const char *operation;
        std::source_location location;

        VkContext(const char *_operation,
                  const std::source_location _location = std::source_location::current())
            : operation(_operation), location(_location) {}
    };

    // Vulkan error checking function
    inline void vkCheck(VkResult result, VkContext ctx) {
        if (result != VK_SUCCESS) {
            std::string_view filename = ctx.location.file_name();
            if (const auto pos = filename.find_last_of("/\\"); pos != std::string_view::npos) {
                filename = filename.substr(pos + 1);
            }

            std::println(std::cerr, "[VULKAN ERROR] {}:{} - {} failed: {}",
                         filename, ctx.location.line(), ctx.operation, string_VkResult(result));
            std::abort();
        }
    }
}