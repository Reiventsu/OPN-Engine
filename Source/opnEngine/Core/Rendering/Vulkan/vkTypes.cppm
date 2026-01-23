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

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vk_mem_alloc.h>

#include <fmt/core.h>

#include "matrix_float_type.h"
#include "vector_float_type.h"


export module opn.Rendering.Util.vkTypes;
import opn.Plugins.ThirdParty.hlslpp;

export
{
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

    //> mat_types
    enum class MaterialPass :uint8_t {
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

    //< mat_types
    //> vbuf_types
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
        glm::mat4 worldMatrix;
        VkDeviceAddress vertexBuffer;
    };

    //< vbuf_types

    //> node_types
    struct DrawContext;

    // base class for a renderable dynamic object
    class IRenderable {
        virtual void Draw(const glm::mat4 &topMatrix, DrawContext &ctx) = 0;
    };

    // implementation of a drawable scene node.
    // the scene node can hold children and will also keep a transform to propagate
    // to them
    struct Node : public IRenderable {
        // parent pointer must be a weak pointer to avoid circular dependencies
        std::weak_ptr<Node> parent;
        std::vector<std::shared_ptr<Node> > children;

        hlslpp::float4x4 localMatrix;
        hlslpp::float4x4 worldMatrix;

        void refreshTransform(const hlslpp::float4x4 &parentMatrix) {
            worldMatrix = parentMatrix * localMatrix;
            for (auto c: children) {
                c->refreshTransform(worldMatrix);
            }
        }

        virtual void Draw(const hlslpp::float4x4 &topMatrix, DrawContext &ctx) {
            // draw children
            for (auto &c: children) {
                c->Draw(topMatrix, ctx);
            }
        }
    };

    //< node_types
    //> intro
#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
             fmt::print("Detected Vulkan error: {}", string_VkResult(err)); \
            abort();                                                    \
        }                                                               \
    } while (0)
    //< intro
}
