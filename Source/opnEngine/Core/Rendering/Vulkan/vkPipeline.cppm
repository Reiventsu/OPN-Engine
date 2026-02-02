module;

#include <filesystem>
#include <fstream>
#include <expected>
#include <volk.h>

export module opn.Rendering.Util.vk.vkPipeline;
import opn.Rendering.Util.vk.vkInit;
import opn.Utils.Logging;
export namespace opn::vkUtil {
    class PipelineBuilder {
    public:
        std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages{};

        VkPipelineInputAssemblyStateCreateInfo m_inputAssembly{};
        VkPipelineRasterizationStateCreateInfo m_rasterizer{};
        VkPipelineColorBlendAttachmentState m_blendAttachment{};
        VkPipelineMultisampleStateCreateInfo m_multisampling{};
        VkPipelineLayout m_pipelineLayout{};
        VkPipelineDepthStencilStateCreateInfo m_depthStencil{};
        VkPipelineRenderingCreateInfo m_renderInfo{};
        VkFormat m_colorAttachmentFormat{};

        PipelineBuilder() { clear(); };

        void clear() {
            m_inputAssembly = {.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
            m_rasterizer = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
            m_blendAttachment = {};
            m_multisampling = {.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
            m_pipelineLayout = {};
            m_depthStencil = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
            m_renderInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
            m_shaderStages.clear();
        }

        VkPipeline buildPipeline(VkDevice _device) {
            VkPipelineViewportStateCreateInfo viewportState{};
            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.pNext = nullptr;

            viewportState.viewportCount = 1;
            viewportState.scissorCount = 1;

            VkPipelineColorBlendStateCreateInfo colorBlendState{};
            colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlendState.pNext = nullptr;

            colorBlendState.attachmentCount = 1;
            colorBlendState.pAttachments = &m_blendAttachment;

            VkPipelineVertexInputStateCreateInfo vertexInputDivisor{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
            };

            VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
            pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineCreateInfo.pNext = &m_renderInfo;

            pipelineCreateInfo.stageCount = static_cast<uint32_t>(m_shaderStages.size());
            pipelineCreateInfo.pStages = m_shaderStages.data();
            pipelineCreateInfo.pVertexInputState = &vertexInputDivisor;
            pipelineCreateInfo.pInputAssemblyState = &m_inputAssembly;
            pipelineCreateInfo.pViewportState = &viewportState;
            pipelineCreateInfo.pRasterizationState = &m_rasterizer;
            pipelineCreateInfo.pMultisampleState = &m_multisampling;
            pipelineCreateInfo.pColorBlendState = &colorBlendState;
            pipelineCreateInfo.pDepthStencilState = &m_depthStencil;
            pipelineCreateInfo.layout = m_pipelineLayout;

            VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
            VkPipelineDynamicStateCreateInfo dynamicInfo{};
            dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicInfo.pDynamicStates = &dynamicStates[0];
            dynamicInfo.dynamicStateCount = 2;

            pipelineCreateInfo.pDynamicState = &dynamicInfo;

            VkPipeline newPipeline;
            if (vkCreateGraphicsPipelines(_device
                                          , VK_NULL_HANDLE
                                          , 1
                                          , &pipelineCreateInfo
                                          , nullptr
                                          , &newPipeline) != VK_SUCCESS) {
                opn::logError("VK Backend (PipelineBuilder)", "Failed to create graphics pipeline:");
                return VK_NULL_HANDLE;
            }
            return newPipeline;
        }

        static std::expected<VkShaderModule, std::string> loadShaderModule(const std::filesystem::path &_filePath,
                                                                           VkDevice _device) {
            std::ifstream file(_filePath, std::ios::ate | std::ios::binary);

            if (!file.is_open()) {
                return std::unexpected(std::format("Failed to open shader: {}", _filePath.string()));
            }

            size_t fileSize = file.tellg();

            std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

            file.seekg(0);
            file.read(reinterpret_cast<char *>(buffer.data()), static_cast<std::streamsize>(fileSize));
            file.close();

            VkShaderModuleCreateInfo createInfo{
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .codeSize = fileSize,
                .pCode = buffer.data()
            };

            VkShaderModule shaderModule;
            if (vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
                return std::unexpected("Vulkan failed to create shader module.");
            }

            return shaderModule;
        }

        VkRenderingAttachmentInfo attachmentInfo(
            VkImageView _view, VkClearValue *_clear,
            VkImageLayout _layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/) {
            VkRenderingAttachmentInfo colorAttachment{};
            colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            colorAttachment.pNext = nullptr;

            colorAttachment.imageView = _view;
            colorAttachment.imageLayout = _layout;
            colorAttachment.loadOp = _clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            if (_clear) {
                colorAttachment.clearValue = *_clear;
            }

            return colorAttachment;
        }

        void setShaders( VkShaderModule _vertexShader, VkShaderModule _fragmentShader ) {
            m_shaderStages.clear();

            m_shaderStages.push_back(
                pipeline_shader_stage_create_info( VK_SHADER_STAGE_VERTEX_BIT, _vertexShader ) );

            m_shaderStages.push_back(
                pipeline_shader_stage_create_info( VK_SHADER_STAGE_FRAGMENT_BIT, _fragmentShader ) );
        }

        void setInputTopology( VkPrimitiveTopology _topology ) {
            m_inputAssembly.topology = _topology;
            m_inputAssembly.primitiveRestartEnable = VK_FALSE;
        }

        void setPolygonMode( VkPolygonMode _mode ) {
            m_rasterizer.polygonMode = _mode;
            m_rasterizer.lineWidth = 1.0f;
        }

        void setCullMode( VkCullModeFlags _mode, VkFrontFace _frontFace ) {
            m_rasterizer.cullMode = _mode;
            m_rasterizer.frontFace = _frontFace;
        }

        void setMultisamplingNone() {
            m_multisampling.sampleShadingEnable = VK_FALSE;
            m_multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            m_multisampling.minSampleShading = 1.0f;
            m_multisampling.pSampleMask = nullptr;

            m_multisampling.alphaToCoverageEnable = VK_FALSE;
            m_multisampling.alphaToOneEnable = VK_FALSE;
        }

        void disableBlending() {
            m_blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                               VK_COLOR_COMPONENT_G_BIT |
                                               VK_COLOR_COMPONENT_B_BIT |
                                               VK_COLOR_COMPONENT_A_BIT;
            m_blendAttachment.blendEnable = VK_FALSE;
        }

        void setColorAttachmentFormat(VkFormat _format) {
            m_colorAttachmentFormat = _format;
            m_renderInfo.colorAttachmentCount = 1;
            m_renderInfo.pColorAttachmentFormats = &m_colorAttachmentFormat;
        }

        void setDepthFormat( VkFormat _format ) {
            m_renderInfo.depthAttachmentFormat = _format;
        }

        void disableDepthTest() {
            m_depthStencil.depthTestEnable = VK_FALSE;
            m_depthStencil.depthWriteEnable = VK_FALSE;
            m_depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
            m_depthStencil.depthBoundsTestEnable = VK_FALSE;
            m_depthStencil.stencilTestEnable = VK_FALSE;
            m_depthStencil.front = {};
            m_depthStencil.back = {};
            m_depthStencil.minDepthBounds = 0.f;
            m_depthStencil.maxDepthBounds = 1.f;
        }
    };
}
