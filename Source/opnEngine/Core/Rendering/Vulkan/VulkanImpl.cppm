module;
#include <atomic>
#include <filesystem>
#include <functional>
#include <thread>
#include <deque>
#include <algorithm>

#include "hlsl++.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
#include "volk.h"
#endif

#include <GLFW/glfw3.h>

#include "VkBootstrap.h"
#include "vk_mem_alloc.h"

export module opn.Renderer.Vulkan;

import opn.Renderer.Backend;
import opn.Renderer.Types;
import opn.System.WindowSurfaceProvider;
import opn.Rendering.Util.vk.vkUtil;
import opn.Utils.Logging;
import opn.Utils.Exceptions;

export namespace opn {
    class VulkanImpl final : public RenderBackend {

        // -- Data --

        std::atomic_bool       m_isInitialized{ };
        uint32_t               m_frameNumber{ };
        WindowSurfaceProvider *m_windowHandle = nullptr;

        VkInstance               m_instance             = nullptr;
        VkDebugUtilsMessengerEXT m_debugMessenger       = nullptr;
        VkPhysicalDevice         m_chosenDevice         = nullptr;
        VkDevice                 m_device               = nullptr;
        VkSurfaceKHR             m_surface              = nullptr;
        VkSwapchainKHR           m_swapchain            = nullptr;

        VkFormat                 m_swapchainImageFormat{ };
        std::vector<VkImage>     m_swapchainImages;
        std::vector<VkImageView> m_swapchainImageViews;
        VkExtent2D               m_swapchainExtent{ };

        vkb::Instance      m_vkbInstance;
        vkb::Device        m_vkbDevice;
        vkb::DispatchTable m_dispatchTable;

        // Debounce
        bool m_pendingResize{ false };
        float m_resizeTimer{0.0f};
        uint32_t m_pendingWidth{0};
        uint32_t m_pendingHeight{0};
        constexpr static float RESIZE_DEBOUNCE_SECONDS = 0.067f;

        struct sDeletionQueue {
            std::deque<std::function<void()>> deleters;

            void pushFunction(std::function<void()>&& function) {
                deleters.emplace_back(std::move(function));
            }

            void flushDeletionQueue() {
                for (auto & deleter : deleters) {
                    deleter();
                }
                deleters.clear();
            }
        };

        struct sFrameData {
            VkSemaphore m_imageAvailableSemaphore{ };
            VkFence     m_inFlightFence{ };

            VkCommandPool   commandPool{ };
            VkCommandBuffer commandBuffer{ };

            sDeletionQueue m_deletionQueue;

        };

        struct sAllocatedImage {
            VkImage       image{ };
            VkImageView   imageView{ };
            VmaAllocation allocation{ };
            VkExtent3D    imageExtent{ };
            VkFormat      format{ };
        };

        struct sComputePushConstants {
            hlslpp::float4 data1;
            hlslpp::float4 data2;
            hlslpp::float4 data3;
            hlslpp::float4 data4;
        };

        constexpr static uint8_t FRAME_OVERLAP = 2;
        sFrameData m_frameData[FRAME_OVERLAP];
        sFrameData &getCurrentFrame() { return m_frameData[m_frameNumber % FRAME_OVERLAP]; }

        sDeletionQueue m_mainDeletionQueue;
        VkQueue        m_graphicsQueue = nullptr;
        uint32_t       m_graphicsQueueFamily = 0;

        VmaAllocator m_vmaAllocator = nullptr;

        sAllocatedImage m_drawImage{ };
        VkExtent2D      m_drawImageExtent{ };

        std::vector< VkFence >     m_imageInFlightFences;
        std::vector< VkSemaphore > m_renderFinishedSemaphores;

        vkUtil::sDescriptorAllocator m_globalDescriptorAllocator{ };

        VkDescriptorSet m_drawImageDescriptors{};
        VkDescriptorSetLayout m_drawImageDescriptorLayout{};

        VkPipeline m_gradientPipeline{ };
        VkPipelineLayout m_gradientPipelineLayout{ };

        VkPipeline m_trianglePipeline{ };
        VkPipelineLayout m_trianglePipelineLayout{ };

        VkPipeline m_meshPipeline{ };
        VkPipelineLayout m_meshPipelineLayout{ };
        vkUtil::sGPUMeshBuffers m_rectangle{ };

        struct sImmediate {
            VkFence fence{ };
            VkCommandBuffer commandBuffer{ };
            VkCommandPool commandPool{ };
        } m_immediate;

        struct sComputeEffect {
            std::string_view name;
            VkPipeline pipeline{ };
            VkPipelineLayout layout{ };
            sComputePushConstants data;
        };

        struct sReflectedPipeline {
            VkPipelineLayout layout{ };
            std::vector<VkDescriptorSetLayout> setLayouts{ };
        };

        std::vector< sComputeEffect > m_backgroundEffects{};
        int32_t m_currentBackgroundEffect = 0;

        std::unordered_map<sPipelineSignature, VkPipelineLayout, sPipelineSignatureHash> m_pipelineLayoutCache;

        // -- Implementation --

        void createInstance() {
            if (volkInitialize() != VK_SUCCESS) {
                throw std::runtime_error("Failed to initialize Volk!");
            }

            vkb::InstanceBuilder builder;
            auto instanceRet = builder
                    .set_app_name( "OPN Engine" )
                    .require_api_version( 1, 3, 0 )
                    .request_validation_layers()
                    .use_default_debug_messenger()
                    .build();

            if (!instanceRet)
                throw std::runtime_error( "Failed to create Vulkan instance!" );

            m_vkbInstance = instanceRet.value();
            m_instance = m_vkbInstance.instance;

            volkLoadInstance(m_instance);

            m_debugMessenger = m_vkbInstance.debug_messenger;

            opn::logDebug( "VulkanBackend", "Vulkan instance created successfully." );
        }

        void createAllocator() {
            opn::logDebug( "VulkanBackend", "Creating VMA Allocator..." );

            if( !m_instance || !m_chosenDevice || !m_device ) {
                opn::logCritical( "VulkanBackend", "VMA prerequisites not met!" );
                throw std::runtime_error( "Cannot create VMA Allocator!" );
            }

            // Provide Volk function pointers to VMA
            VmaVulkanFunctions vulkanFunctions = { };
            vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
            vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

            VmaAllocatorCreateInfo allocatorCreateInfo = { };
            allocatorCreateInfo.physicalDevice = m_chosenDevice;
            allocatorCreateInfo.device = m_device;
            allocatorCreateInfo.instance = m_instance;
            allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
            allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
            allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

            vkUtil::vkCheck(
                vmaCreateAllocator( &allocatorCreateInfo, &m_vmaAllocator ),
                "vmaCreateAllocator"
            );

            opn::logDebug( "VulkanBackend", "VMA Allocator created successfully!" );
        }

        void createDevices() {

            VkPhysicalDeviceVulkan13Features features13 = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES
            };
            features13.dynamicRendering = VK_TRUE;
            features13.synchronization2 = VK_TRUE;

            VkPhysicalDeviceVulkan12Features features12 = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES
            };
            features12.bufferDeviceAddress = VK_TRUE;
            features12.descriptorIndexing = VK_TRUE;
            VkPhysicalDeviceVulkan11Features features11 = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES
            };
            features11.shaderDrawParameters = VK_TRUE;
            VkPhysicalDeviceFeatures features1 = { };
            features1.shaderInt64 = VK_TRUE;

            vkb::PhysicalDeviceSelector selector{m_vkbInstance};
            auto physicalDeviceRet = selector
                    .set_minimum_version(1, 3)
                    .set_required_features(features1)
                    .set_required_features_11(features11)
                    .set_required_features_12(features12)
                    .set_required_features_13(features13)
                    .set_surface(m_surface)
                    .select();

            if (!physicalDeviceRet) {
                opn::logCritical("VulkanBackend", "Failed to find suitable GPU!");
                throw std::runtime_error("Failed to find suitable GPU!");
            }

            const vkb::PhysicalDevice& physicalDevice = physicalDeviceRet.value();

            vkb::DeviceBuilder deviceBuilder{physicalDevice};
            auto deviceRet = deviceBuilder.build();

            if (!deviceRet) {
                opn::logCritical("VulkanBackend", "Failed to create logical device!");
                throw std::runtime_error("Failed to create logical device!");
            }

            m_vkbDevice = deviceRet.value();
            m_device = m_vkbDevice.device;
            m_chosenDevice = physicalDevice.physical_device;

            volkLoadDevice(m_device);

            opn::logDebug("VulkanBackend", "Vulkan device created successfully.");
        }

        void createSwapchain() {

            auto [ width, height ] = m_windowHandle->dimension;
            if( width == 0 || height == 0 ) {
                destroySwapchain();
                return;
            }

            VkSurfaceCapabilitiesKHR capabilities;
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR( m_chosenDevice, m_surface, &capabilities );

            if( capabilities.currentExtent.width == 0 ||
                capabilities.currentExtent.height == 0 ) {
                destroySwapchain();
                return;
            }

            vkDeviceWaitIdle( m_device );

            vkb::SwapchainBuilder swapchainBuilder{ m_chosenDevice, m_device, m_surface };
            m_swapchainImageFormat = VK_FORMAT_R8G8B8A8_UNORM;

            if( m_swapchain != VK_NULL_HANDLE ) {
                VkSwapchainKHR oldSwapchain = m_swapchain;
                swapchainBuilder.set_old_swapchain( oldSwapchain );
            }

            auto vkbSwapChainRet = swapchainBuilder
            .set_desired_format( VkSurfaceFormatKHR{
                .format = m_swapchainImageFormat,
                .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
            } )
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent( width, height )
            .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .set_old_swapchain( m_swapchain )
            .build();

            if ( !vkbSwapChainRet ) {
                opn::logCritical("VulkanBackend", "Failed to create swapchain: {}",
                    vkbSwapChainRet.error().message());
                destroySwapchain();
                return;
            }

            vkb::Swapchain vkbSwapChain = vkbSwapChainRet.value();

            if( vkbSwapChainRet->extent.width == 0 || vkbSwapChainRet->extent.height == 0 ) {
                vkb::destroy_swapchain( vkbSwapChain );
                return;
            }

            for( const auto view : m_swapchainImageViews ) {
                if( view != VK_NULL_HANDLE ) {
                    vkDestroyImageView( m_device, view, nullptr );
                }
            }
            m_swapchainImageViews.clear();

            for( const auto semaphore : m_renderFinishedSemaphores ) {
                if( semaphore != VK_NULL_HANDLE ) {
                    vkDestroySemaphore( m_device, semaphore, nullptr );
                }
            }
            m_renderFinishedSemaphores.clear();

            if( m_swapchain != VK_NULL_HANDLE ) {
                vkDestroySwapchainKHR( m_device, m_swapchain, nullptr );
            }

            m_swapchain                 = vkbSwapChain.swapchain;
            m_swapchainExtent           = vkbSwapChain.extent;
            m_swapchainImages           = vkbSwapChain.get_images().value();
            m_swapchainImageViews       = vkbSwapChain.get_image_views().value();

            const size_t imageCount = m_swapchainImages.size();
            m_renderFinishedSemaphores.assign(imageCount, VK_NULL_HANDLE);
            m_imageInFlightFences.assign(imageCount, VK_NULL_HANDLE);

            VkSemaphoreCreateInfo semaphoreCreateInfo = vkUtil::semaphore_create_info();
            for (size_t i = 0; i < imageCount; i++) {
                vkUtil::vkCheck(
                    vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_renderFinishedSemaphores[i]),
                    "vkCreateSemaphore: renderFinished"
                );
            }

            opn::logDebug("VulkanBackend", "Swapchain created: {}x{}.",
                    m_swapchainExtent.width, m_swapchainExtent.height);

            VkExtent3D drawImageExtent = { m_swapchainExtent.width, m_swapchainExtent.height, 1 };

            if( m_drawImage.imageView != VK_NULL_HANDLE ) {
                vkDestroyImageView( m_device
                                  , m_drawImage.imageView
                                  , nullptr
                );
                m_drawImage.imageView = VK_NULL_HANDLE;
            }

            if( m_drawImage.image != VK_NULL_HANDLE ) {
                vmaDestroyImage( m_vmaAllocator
                               , m_drawImage.image
                               , m_drawImage.allocation
                );
                m_drawImage.image      = VK_NULL_HANDLE;
                m_drawImage.allocation = VK_NULL_HANDLE;
            }

            m_drawImage.format = VK_FORMAT_R16G16B16A16_SFLOAT;
            m_drawImage.imageExtent = drawImageExtent;
            m_drawImageExtent = {drawImageExtent.width, drawImageExtent.height};

            VkImageUsageFlags drawImageUsages {};
            drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
            drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            VkImageCreateInfo rimgInfo = vkUtil::image_create_info( m_drawImage.format
                                                                  , drawImageUsages
                                                                  , drawImageExtent
            );

            VmaAllocationCreateInfo rimgAllocInfo = {
                .usage = VMA_MEMORY_USAGE_GPU_ONLY,
                .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
            };

            vmaCreateImage( m_vmaAllocator
                          , &rimgInfo
                          , &rimgAllocInfo
                          , &m_drawImage.image
                          , &m_drawImage.allocation
                          , nullptr
            );

            VkImageViewCreateInfo rViewInfo = vkUtil::imageview_create_info( m_drawImage.format
                                                                           , m_drawImage.image
                                                                           , VK_IMAGE_ASPECT_COLOR_BIT
            );

            vkUtil::vkCheck(
                vkCreateImageView( m_device
                                 , &rViewInfo
                                 , nullptr
                                 , &m_drawImage.imageView )
                                 , "vkCreateImageView"
            );

            if( m_drawImageDescriptors != VK_NULL_HANDLE ) {
                VkDescriptorImageInfo imageInfo{};
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                imageInfo.imageView = m_drawImage.imageView;

                VkWriteDescriptorSet drawImageWrite{};
                drawImageWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                drawImageWrite.pNext           = nullptr;
                drawImageWrite.dstBinding      = 0;
                drawImageWrite.dstSet          = m_drawImageDescriptors;
                drawImageWrite.descriptorCount = 1;
                drawImageWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                drawImageWrite.pImageInfo      = &imageInfo;

                vkUpdateDescriptorSets( m_device
                                      , 1
                                      , &drawImageWrite
                                      , 0
                                      , nullptr
                );
            }
        }

        void createCommands() {
            opn::logDebug("VulkanBackend", "Creating command pools...");
            VkCommandPoolCreateInfo commandPoolInfo =
                    vkUtil::command_pool_create_info( m_graphicsQueueFamily
                                                    , VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT );

            for (auto &i: m_frameData) {
                vkUtil::vkCheck(
                    vkCreateCommandPool( m_device, &commandPoolInfo, nullptr, &i.commandPool )
                                       , "vkCreateCommandPool"
                );

                VkCommandBufferAllocateInfo cmdAllocInfo = vkUtil::command_buffer_allocate_info(
                    i.commandPool, 1
                );

                vkUtil::vkCheck(
                    vkAllocateCommandBuffers( m_device, &cmdAllocInfo, &i.commandBuffer )
                                            , "vkAllocateCommandBuffers"
                );
            }

            vkUtil::vkCheck(
                vkCreateCommandPool( m_device, &commandPoolInfo, nullptr, &m_immediate.commandPool )
                                   , "vkCreateCommandPool"
            );

            VkCommandBufferAllocateInfo cmdAllocInfo =
                vkUtil::command_buffer_allocate_info( m_immediate.commandPool, 1 );

            vkUtil::vkCheck(
                vkAllocateCommandBuffers( m_device, &cmdAllocInfo, &m_immediate.commandBuffer )
                                        , "vkAllocateCommandBuffers"
            );

            m_mainDeletionQueue.pushFunction( [ this ] {
                vkDestroyCommandPool( m_device, m_immediate.commandPool, nullptr );
            } );

            opn::logDebug("VulkanBackend", "Command pools created.");
        }

        void createSyncObjects() {
            opn::logDebug( "VulkanBackend", "Creating sync objects..." );

            VkFenceCreateInfo     fenceCreateInfo     = vkUtil::fence_create_info( VK_FENCE_CREATE_SIGNALED_BIT );
            VkSemaphoreCreateInfo semaphoreCreateInfo = vkUtil::semaphore_create_info();

            for( auto &i: m_frameData ) {
                vkUtil::vkCheck(
                    vkCreateFence(m_device, &fenceCreateInfo, nullptr, &i.m_inFlightFence),
                    "vkCreateFence"
                );

                vkUtil::vkCheck(
                    vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &i.m_imageAvailableSemaphore),
                    "vkCreateSemaphore: imageAvailable"
                );
            }

            vkUtil::vkCheck(
                vkCreateFence( m_device, &fenceCreateInfo, nullptr, &m_immediate.fence )
                             , "vkCreateFence"
            );

            m_mainDeletionQueue.pushFunction( [ this ] {
                vkDestroyFence( m_device, m_immediate.fence, nullptr );
            } );

            opn::logDebug("VulkanBackend", "Sync objects created for {} frames.",
                 FRAME_OVERLAP );
        }

        void createDescriptors() {
            opn::logDebug("VulkanBackend", "Creating descriptor objects...");

            std::vector< vkUtil::sDescriptorAllocator::sPoolSizeRatio > sizes = {
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.0f }
            };

            m_globalDescriptorAllocator.initPool( m_device, 10, sizes );

            {
                vkUtil::sDescriptorLayoutBuilder builder;
                builder.add_binding( 0
                                   , VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
                );

                m_drawImageDescriptorLayout = builder.build( m_device
                                                           , VK_SHADER_STAGE_COMPUTE_BIT
                );
            }

            m_drawImageDescriptors = m_globalDescriptorAllocator.allocate( m_device
                                                                         , m_drawImageDescriptorLayout
            );

            m_mainDeletionQueue.pushFunction( [ this ]( ) {
                m_globalDescriptorAllocator.destroyPool(m_device);
                vkDestroyDescriptorSetLayout( m_device, m_drawImageDescriptorLayout, nullptr);
            });
        }

        vkUtil::sAllocatedBuffer createBuffer(size_t _allocSize, VkBufferUsageFlags _usage, VmaMemoryUsage _memUsage) {
            VkBufferCreateInfo bufferInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
            bufferInfo.pNext = nullptr;
            bufferInfo.size = _allocSize;

            bufferInfo.usage = _usage;

            VmaAllocationCreateInfo vmaAllocInfo = { };
            vmaAllocInfo.usage = _memUsage;
            vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
            vkUtil::sAllocatedBuffer newBuffer;

            vkUtil::vkCheck(
                vmaCreateBuffer( m_vmaAllocator
                               , &bufferInfo
                               , &vmaAllocInfo
                               , &newBuffer.buffer
                               , &newBuffer.allocation
                               , nullptr)
                               , "vmaCreateBuffer"
            );

            return newBuffer;
        }

        void createPipelines() {
            // compute pipelines
            createBackgroundPipelines();

            // graphics pipelines
            createTrianglePipeleine();
            createMeshPipeline();
        }

        void createDefaultData() {
            std::array<vkUtil::sVertex, 4> rectVertices;
            rectVertices[0].position = {0.5,-0.5, 0};
            rectVertices[1].position = {0.5,0.5, 0};
            rectVertices[2].position = {-0.5,-0.5, 0};
            rectVertices[3].position = {-0.5,0.5, 0};

            rectVertices[0].color = {0,0, 0,1};
            rectVertices[1].color = { 0.5,0.5,0.5 ,1};
            rectVertices[2].color = { 1,0, 0,1 };
            rectVertices[3].color = { 0,1, 0,1 };

            std::array<uint32_t,6> rectIndices;
            rectIndices[0] = 0;
            rectIndices[1] = 1;
            rectIndices[2] = 2;
            rectIndices[3] = 2;
            rectIndices[4] = 1;
            rectIndices[5] = 3;

            m_rectangle = uploadMesh(rectIndices, rectVertices);

            m_mainDeletionQueue.pushFunction([this] {
                destroyBuffer(m_rectangle.indexBuffer);
                destroyBuffer(m_rectangle.vertexBuffer);
            });

        }

        void createBackgroundPipelines() {
            opn::logDebug("VulkanBackend", "Creating background pipelines...");

            VkPipelineLayoutCreateInfo computeLayout{ };
            computeLayout.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            computeLayout.pNext          = nullptr;
            computeLayout.pSetLayouts    = &m_drawImageDescriptorLayout;
            computeLayout.setLayoutCount = 1;

            VkPushConstantRange pushConstantRange{};
            pushConstantRange.offset = 0;
            pushConstantRange.size = sizeof( sComputePushConstants );
            pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

            computeLayout.pPushConstantRanges = &pushConstantRange;
            computeLayout.pushConstantRangeCount = 1;

            VkPipelineLayout sharedLayout{};
            vkUtil::vkCheck( vkCreatePipelineLayout( m_device
                                                   , &computeLayout
                                                   , nullptr
                                                   , &sharedLayout)
                                                   , "vkCreatePipelineLayout"
            );

            std::filesystem::path gradientPath = std::filesystem::current_path() / ".." / "Shaders" / "gradientColour.comp.spv";
            auto gradientResult = vkUtil::PipelineBuilder::loadShaderModule(gradientPath, m_device);

            if (!gradientResult) {
                opn::logError("VulkanBackend", "Failed to load gradient shader: {}", gradientResult.error());
                vkDestroyPipelineLayout(m_device, sharedLayout, nullptr);
                return;
            }
            VkShaderModule gradientShader = gradientResult.value();

            std::filesystem::path skyPath = std::filesystem::current_path() / ".." / "Shaders" / "sky.comp.spv";
            auto skyResult = vkUtil::PipelineBuilder::loadShaderModule(skyPath, m_device);

            if (!skyResult) {
                opn::logError("VulkanBackend", "Failed to load sky shader: {}", skyResult.error());
                vkDestroyShaderModule(m_device, gradientShader, nullptr);
                vkDestroyPipelineLayout(m_device, sharedLayout, nullptr);
                return;
            }
            VkShaderModule skyShader = skyResult.value();

            {
                VkPipelineShaderStageCreateInfo stageInfo{};
                stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                stageInfo.pNext = nullptr;
                stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
                stageInfo.module = gradientShader;
                stageInfo.pName  = "main";

                VkComputePipelineCreateInfo computePipelineCreateInfo{};
                computePipelineCreateInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
                computePipelineCreateInfo.pNext  = nullptr;
                computePipelineCreateInfo.layout = sharedLayout;
                computePipelineCreateInfo.stage  = stageInfo;

                sComputeEffect gradient;
                gradient.name = "gradient";
                gradient.layout = sharedLayout;
                gradient.data.data1 = hlslpp::float4(1.0f, 0.0f, 0.0f, 1.0f);
                gradient.data.data2 = hlslpp::float4(0.0f, 0.0f, 1.0f, 1.0f);

                vkUtil::vkCheck(
                    vkCreateComputePipelines( m_device
                                            , VK_NULL_HANDLE
                                            , 1
                                            , &computePipelineCreateInfo
                                            , nullptr
                                            , &gradient.pipeline)
                                            , "vkCreateComputePipelines (gradient)"
                );

                m_backgroundEffects.push_back(std::move(gradient));
            }

            {
                VkPipelineShaderStageCreateInfo stageInfo{};
                stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                stageInfo.pNext = nullptr;
                stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
                stageInfo.module = skyShader;
                stageInfo.pName = "main";

                VkComputePipelineCreateInfo pipelineInfo{};
                pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
                pipelineInfo.pNext = nullptr;
                pipelineInfo.layout = sharedLayout;
                pipelineInfo.stage = stageInfo;

                sComputeEffect sky;
                sky.name = "sky";
                sky.layout = sharedLayout;
                sky.data.data1 = hlslpp::float4(0.1f, 0.2f, 0.4f, 0.97f);

                vkUtil::vkCheck(
                    vkCreateComputePipelines( m_device
                                            , VK_NULL_HANDLE
                                            , 1
                                            , &pipelineInfo
                                            , nullptr
                                            , &sky.pipeline)
                                            , "vkCreateComputePipelines (sky)"
                );

                m_backgroundEffects.push_back(std::move(sky));
            }

            vkDestroyShaderModule(m_device, gradientShader, nullptr);
            vkDestroyShaderModule(m_device, skyShader, nullptr);

            m_mainDeletionQueue.pushFunction([this, sharedLayout]() {
                vkDestroyPipelineLayout(m_device, sharedLayout, nullptr);

                for (auto& effect : m_backgroundEffects) {
                    vkDestroyPipeline(m_device, effect.pipeline, nullptr);
                }
            });
        }

        void createTrianglePipeleine() {
            opn::logDebug("VulkanBackend", "Creating triangle pipeline...");

            VkShaderModule triangleFragShader;
            auto triangleFragShaderResult
            = vkUtil::PipelineBuilder::loadShaderModule(std::filesystem::current_path() / ".." / "Shaders" / "triangle.frag.spv", m_device);
            if (!triangleFragShaderResult) {
                opn::logError("VulkanBackend", "Failed to load triangle shader: {}", triangleFragShaderResult.error());
                return;
            }
            triangleFragShader = triangleFragShaderResult.value();

            VkShaderModule triangleVertShader;
            auto triangleVertShaderResult
            = vkUtil::PipelineBuilder::loadShaderModule(std::filesystem::current_path() / ".." / "Shaders" / "triangle.vert.spv", m_device);
            if (!triangleVertShaderResult) {
                opn::logError("VulkanBackend", "Failed to load triangle shader: {}", triangleVertShaderResult.error());
                return;
            }
            triangleVertShader = triangleVertShaderResult.value();

            VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkUtil::pipeline_layout_create_info();
            vkUtil::vkCheck(
                vkCreatePipelineLayout( m_device
                                      , &pipelineLayoutInfo
                                      , nullptr
                                      , &m_trianglePipelineLayout )
                                      , "vkCreatePipelineLayout"
            );

            vkUtil::PipelineBuilder pipelineBuilder;

            pipelineBuilder.m_pipelineLayout = m_trianglePipelineLayout;
            pipelineBuilder.setShaders( triangleVertShader, triangleFragShader );
            pipelineBuilder.setInputTopology( VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST );
            pipelineBuilder.setPolygonMode( VK_POLYGON_MODE_FILL );
            pipelineBuilder.setCullMode( VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
            pipelineBuilder.setMultisamplingNone();
            pipelineBuilder.disableBlending();
            pipelineBuilder.disableDepthTest();

            pipelineBuilder.setColorAttachmentFormat( m_drawImage.format );
            pipelineBuilder.setDepthFormat( VK_FORMAT_UNDEFINED );

            m_trianglePipeline = pipelineBuilder.buildPipeline( m_device );

            vkDestroyShaderModule( m_device, triangleFragShader, nullptr );
            vkDestroyShaderModule( m_device, triangleVertShader, nullptr );

            m_mainDeletionQueue.pushFunction( [ this ] {
                vkDestroyPipelineLayout( m_device, m_trianglePipelineLayout, nullptr );
                vkDestroyPipeline(m_device, m_trianglePipeline, nullptr);
            });
        }

        void createMeshPipeline() {
            VkShaderModule meshTriangleFragShader;
            auto meshTriangleFragShaderResult
            = vkUtil::PipelineBuilder::loadShaderModule(std::filesystem::current_path() / ".." / "Shaders" / "coloured_triangle.frag.spv", m_device);
            if (!meshTriangleFragShaderResult) {
                opn::logError("VulkanBackend", "Failed to load mesh triangle shader: {}", meshTriangleFragShaderResult.error());
                return;
            }
            meshTriangleFragShader = meshTriangleFragShaderResult.value();

            VkShaderModule meshTriangleVertShader;
            auto meshTriangleVertShaderResult
            = vkUtil::PipelineBuilder::loadShaderModule(std::filesystem::current_path() / ".." / "Shaders" / "coloured_triangle.vert.spv", m_device);
            if (!meshTriangleVertShaderResult) {
                opn::logError("VulkanBackend", "Failed to load mesh triangle shader: {}", meshTriangleVertShaderResult.error());
            }
            meshTriangleVertShader = meshTriangleVertShaderResult.value();

            VkPushConstantRange pushConstantRange{};
            pushConstantRange.offset = 0;
            pushConstantRange.size = sizeof( vkUtil::sGPUDrawPushConstants );
            pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

            VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkUtil::pipeline_layout_create_info();
            pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
            pipelineLayoutInfo.pushConstantRangeCount = 1;

            vkUtil::vkCheck(
                vkCreatePipelineLayout( m_device
                                      , &pipelineLayoutInfo
                                      , nullptr
                                      , &m_meshPipelineLayout )
                                      , "vkCreatePipelineLayout"
            );

            vkUtil::PipelineBuilder meshPipelineBuilder;
            meshPipelineBuilder.m_pipelineLayout = m_meshPipelineLayout;
            meshPipelineBuilder.setShaders( meshTriangleVertShader, meshTriangleFragShader );
            meshPipelineBuilder.setInputTopology( VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST );
            meshPipelineBuilder.setPolygonMode( VK_POLYGON_MODE_FILL );
            meshPipelineBuilder.setCullMode( VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
            meshPipelineBuilder.setMultisamplingNone();
            meshPipelineBuilder.disableBlending();
            meshPipelineBuilder.disableDepthTest();

            meshPipelineBuilder.setColorAttachmentFormat( m_drawImage.format );
            meshPipelineBuilder.setDepthFormat( VK_FORMAT_UNDEFINED );

            m_meshPipeline = meshPipelineBuilder.buildPipeline( m_device );

            vkDestroyShaderModule( m_device, meshTriangleFragShader, nullptr );
            vkDestroyShaderModule( m_device, meshTriangleVertShader, nullptr );

            m_mainDeletionQueue.pushFunction( [ this ] {
                vkDestroyPipelineLayout( m_device, m_meshPipelineLayout, nullptr );
                vkDestroyPipeline(m_device, m_meshPipeline, nullptr);
            });
        }

        void createImGui() {
            opn::logDebug("VulkanBackend", "Creating ImGui objects...");
            VkDescriptorPoolSize poolSizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 100 },
                                                  { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 },
                                                  { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100 },
                                                  { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100 },
                                                  { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 100 },
                                                  { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 100 },
                                                  { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
                                                  { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 },
                                                  { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100 },
                                                  { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100 },
                                                  { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 100 }
            };

            VkDescriptorPoolCreateInfo descriptorPoolInfo = {};

            descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            descriptorPoolInfo.maxSets = 1000;
            descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
            descriptorPoolInfo.pPoolSizes = poolSizes;

            VkDescriptorPool imguiPool;
            vkUtil::vkCheck(
                vkCreateDescriptorPool( m_device
                                      , &descriptorPoolInfo
                                      , nullptr
                                      , &imguiPool )
                                      , "vkCreateDescriptorPool"
            );

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();

            auto* glfwWindow = m_windowHandle->getGLFWWindow();
            if ( !glfwWindow ) {
                opn::logCritical("VulkanBackend", "Failed to get native window handle. ImGui will not work.");
                return;
            }
            ImGui_ImplGlfw_InitForVulkan( glfwWindow, true);

            ImGui_ImplVulkan_InitInfo initInfo{};
            initInfo.ApiVersion = VK_API_VERSION_1_3;
            initInfo.Instance = m_instance;
            initInfo.PhysicalDevice = m_chosenDevice;
            initInfo.Device = m_device;
            initInfo.Queue = m_graphicsQueue;
            initInfo.DescriptorPool = imguiPool;
            initInfo.MinImageCount = 3;
            initInfo.ImageCount = 3;
            initInfo.UseDynamicRendering = true;

            initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
            initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
            initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_swapchainImageFormat;

            initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

            ImGui_ImplVulkan_Init( &initInfo );

            m_mainDeletionQueue.pushFunction( [ this, imguiPool ] {
                ImGui_ImplVulkan_Shutdown();
                vkDestroyDescriptorPool( m_device, imguiPool, nullptr );
            });
            opn::logDebug("VulkanBackend", "ImGui objects created.");
        }

        VkPipelineLayout getOrCreatePipelineLayout(const sShaderReflection& _reflection) {
            const auto sig = sPipelineSignature::from(_reflection);

            if (const auto itr = m_pipelineLayoutCache.find(sig); itr != m_pipelineLayoutCache.end() ) {
                return itr->second;
            }

            auto sortedSets = _reflection.setLayouts;
            std::ranges::sort(sortedSets, {}, &sSetLayoutData::setIndex);;

            std::vector<VkDescriptorSetLayout> setLayouts;
            setLayouts.reserve(sortedSets.size());

            for (const auto& setData : sortedSets) {
                VkDescriptorSetLayoutCreateInfo info {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                    .bindingCount = static_cast<uint32_t>(setData.bindings.size()),
                    .pBindings = setData.bindings.data()
                };
                VkDescriptorSetLayout layout;
                vkUtil::vkCheck(vkCreateDescriptorSetLayout(m_device, &info, nullptr, &layout),
                               "vkCreateDescriptorSetLayout (reflection)"
                );
                setLayouts.push_back(layout);
            };

            VkPipelineLayoutCreateInfo layoutInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount = static_cast<uint32_t>(setLayouts.size()),
                .pSetLayouts = setLayouts.data(),
                .pushConstantRangeCount = static_cast<uint32_t>(_reflection.pushConstants.size()),
                .pPushConstantRanges = _reflection.pushConstants.data()
            };

            VkPipelineLayout pipelineLayout;
            vkUtil::vkCheck(vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &pipelineLayout),
                           "vkCreatePipelineLayout (reflection)"
            );

            m_pipelineLayoutCache.emplace(sig, pipelineLayout);

            for( auto dsl : setLayouts) {
                 m_mainDeletionQueue.pushFunction([this, dsl]{vkDestroyDescriptorSetLayout(m_device, dsl, nullptr); });
                 m_mainDeletionQueue.pushFunction([this, pipelineLayout]{ vkDestroyPipelineLayout(m_device, pipelineLayout, nullptr); });
            }

            return pipelineLayout;
        }

        void destroySwapchain() {
            if( m_device != VK_NULL_HANDLE ) {
                vkDeviceWaitIdle( m_device );
            }

            for (auto semaphore : m_renderFinishedSemaphores) {
                if (semaphore != VK_NULL_HANDLE) {
                    vkDestroySemaphore(m_device, semaphore, nullptr);
                }
            }
            m_renderFinishedSemaphores.clear();

            for( const auto view : m_swapchainImageViews ) {
                if( view != VK_NULL_HANDLE ) {
                    vkDestroyImageView( m_device, view, nullptr );
                }
            }
            m_swapchainImageViews.clear();

            if( m_swapchain != VK_NULL_HANDLE ) {
                vkDestroySwapchainKHR( m_device, m_swapchain, nullptr );
                m_swapchain = VK_NULL_HANDLE;
            }

            m_swapchainImages.clear();
            m_imageInFlightFences.clear();
        }

        void destroyBuffer(const vkUtil::sAllocatedBuffer& _buffer) const {
            vmaDestroyBuffer( m_vmaAllocator, _buffer.buffer, _buffer.allocation );
        }

    public:
        vkUtil::sGPUMeshBuffers uploadMesh(std::span<uint32_t> _indices, std::span<vkUtil::sVertex> _vertices) {
            vkUtil::sGPUMeshBuffers newSurface{};

            size_t vertexBufferSize = _vertices.size() * sizeof(vkUtil::sVertex);
            size_t indexBufferSize = _indices.size() * sizeof(uint32_t);

            newSurface.vertexBuffer = createBuffer( vertexBufferSize
                                                  , VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                                                  ,VMA_MEMORY_USAGE_GPU_ONLY
            );

            VkBufferDeviceAddressInfo bufferDeviceAddressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
            bufferDeviceAddressInfo.buffer = newSurface.vertexBuffer.buffer;
            newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(m_device, &bufferDeviceAddressInfo);

            newSurface.indexBuffer = createBuffer( indexBufferSize
                                                 , VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
                                                 , VMA_MEMORY_USAGE_GPU_ONLY
            );

            vkUtil::sAllocatedBuffer staging = createBuffer( vertexBufferSize + indexBufferSize
                                                          , VK_BUFFER_USAGE_TRANSFER_SRC_BIT
                                                          , VMA_MEMORY_USAGE_CPU_ONLY
            );

            VmaAllocationInfo stagingAllocInfo{};
            vmaGetAllocationInfo( m_vmaAllocator, staging.allocation, &stagingAllocInfo );

            void* data = stagingAllocInfo.pMappedData;
            memcpy(data, _vertices.data(), vertexBufferSize);
            memcpy(static_cast<char *>(data) + vertexBufferSize, _indices.data(), indexBufferSize);

            submitImmediate([ & ] (VkCommandBuffer _command) {
                VkBufferCopy vertexCopy{ 0 };
                vertexCopy.srcOffset = 0;
                vertexCopy.dstOffset = 0;
                vertexCopy.size = vertexBufferSize;

                vkCmdCopyBuffer( _command, staging.buffer, newSurface.vertexBuffer.buffer, 1, &vertexCopy );

                VkBufferCopy indexCopy{ 0 };
                indexCopy.srcOffset = vertexBufferSize;
                indexCopy.dstOffset = 0;
                indexCopy.size = indexBufferSize;

                vkCmdCopyBuffer( _command, staging.buffer, newSurface.indexBuffer.buffer, 1, &indexCopy );
            });

            destroyBuffer( staging );
            return newSurface;
        }

        vkUtil::sGPUMeshStreams uploadMeshStreams(
            std::span<const float>    _positions,
            std::span<const float>    _normals,
            std::span<const float>    _uvs,
            std::span<const uint32_t> _indices
        ) {
            vkUtil::sGPUMeshStreams streams{};

            const size_t posSize = _positions.size_bytes();
            const size_t normalSize = _normals.size_bytes();
            const size_t uvSize = _uvs.size_bytes();
            const size_t indicesSize = _indices.size_bytes();
            const size_t stagingSize = posSize + normalSize + uvSize + indicesSize;

            vkUtil::sAllocatedBuffer staging = createBuffer(
                stagingSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VMA_MEMORY_USAGE_CPU_ONLY
            );

            VmaAllocationInfo stagingAllocInfo{};
            vmaGetAllocationInfo(m_vmaAllocator, staging.allocation, &stagingAllocInfo);
            char* dst = static_cast<char*>(stagingAllocInfo.pMappedData);

            memcpy(dst, _positions.data(), posSize);
            memcpy(dst + posSize, _normals.data(), normalSize);
            memcpy(dst + posSize + normalSize, _uvs.data(), uvSize);
            memcpy(dst + posSize + normalSize + uvSize, _indices.data(), indicesSize);

            streams.positions = createBuffer(
                posSize,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY
            );

            streams.normals = createBuffer(
                normalSize,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY
            );

            streams.uvs = createBuffer(
                uvSize,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY
            );

            streams.indices = createBuffer(
                indicesSize,
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY
            );

            submitImmediate([&](VkCommandBuffer _cmd){
                VkBufferCopy copy{};

                copy = { .srcOffset = 0, .dstOffset = 0, .size = posSize };
                vkCmdCopyBuffer(_cmd, staging.buffer, streams.positions.buffer, 1, &copy);

                copy = { .srcOffset = posSize, .dstOffset = 0, .size = normalSize };
                vkCmdCopyBuffer(_cmd, staging.buffer, streams.normals.buffer, 1, &copy);

                copy = { .srcOffset = posSize + normalSize, .dstOffset = 0, .size = uvSize };
                vkCmdCopyBuffer(_cmd, staging.buffer, streams.uvs.buffer, 1, &copy);

                copy = { .srcOffset = posSize + normalSize + uvSize, .dstOffset = 0, .size = indicesSize };
                vkCmdCopyBuffer(_cmd, staging.buffer, streams.indices.buffer, 1, &copy);
            });

            destroyBuffer( staging );
            return streams;
        }

        void init() override {
            if (m_isInitialized.exchange(true))
                throw MultipleInit_Exception("VulkanBackend: Multiple init calls on graphics backend!");
            opn::logDebug("VulkanBackend", "Initializing...");
            createInstance();
        }

        void completeInit() {
            opn::logDebug("VulkanBackend", "Completing initialization...");

            createDevices();
            createAllocator();
            createDescriptors();
            createSwapchain();

            opn::logDebug("VulkanBackend", "Creating queues...");
            m_graphicsQueue = m_vkbDevice.get_queue(vkb::QueueType::graphics).value();
            m_graphicsQueueFamily = m_vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

            createCommands();
            createSyncObjects();

            createDefaultData();
            createPipelines();
            createImGui();

            opn::logDebug("VulkanBackend", "Initialization complete.");
        }

        void shutdown() override {
            logDebug("VulkanBackend", "Shutting down...");
            if (m_isInitialized.exchange(false)) {
                vkDeviceWaitIdle(m_device);

                m_mainDeletionQueue.flushDeletionQueue();
                for ( auto&i:m_frameData) {
                    i.m_deletionQueue.flushDeletionQueue();
                }

                if( m_drawImage.imageView ) {
                    vkDestroyImageView( m_device
                                      , m_drawImage.imageView
                                      , nullptr
                    );
                    m_drawImage.imageView = VK_NULL_HANDLE;
                }
                if( m_drawImage.image ) {
                    vmaDestroyImage( m_vmaAllocator
                                   , m_drawImage.image
                                   , m_drawImage.allocation
                    );
                    m_drawImage.image = VK_NULL_HANDLE;
                    m_drawImage.allocation = VK_NULL_HANDLE;
                }

                if (m_vmaAllocator) {
                    vmaDestroyAllocator(m_vmaAllocator);
                    m_vmaAllocator = nullptr;
                }

                destroySwapchain();

                for (const auto &i: m_frameData) {
                    vkDestroyCommandPool(m_device, i.commandPool, nullptr);
                    vkDestroyFence(m_device, i.m_inFlightFence, nullptr);
                    vkDestroySemaphore(m_device, i.m_imageAvailableSemaphore, nullptr);
                }

                for (auto semaphore : m_renderFinishedSemaphores ) {
                    vkDestroySemaphore(m_device, semaphore, nullptr);
                }
                m_renderFinishedSemaphores.clear();

                for ( auto i : m_frameData ) {
                    i.m_deletionQueue.flushDeletionQueue();
                }
                m_imageInFlightFences.clear();

                m_mainDeletionQueue.flushDeletionQueue();

                vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
                vkDestroyDevice(m_device, nullptr);

                vkb::destroy_debug_utils_messenger(m_instance, m_debugMessenger);
                vkDestroyInstance(m_instance, nullptr);
            } else {
                logWarning("VulkanBackend", "Shutdown called, but backend was not initialized. Ignoring.");
            }
            opn::logDebug("VulkanBackend", "Shutdown complete.");
        }

        void update( const float _deltaTime ) override {
            if (m_isInitialized == false) return;

            int glfwWidth, glfwHeight;
            glfwGetFramebufferSize( m_windowHandle->getGLFWWindow(), &glfwWidth, &glfwHeight );
            const auto width  = static_cast< uint32_t >( glfwWidth  );
            const auto height = static_cast< uint32_t >( glfwHeight );

            const bool sizeMismatch = ( width != m_pendingWidth || height != m_pendingHeight );

            if( sizeMismatch || (m_swapchain == VK_NULL_HANDLE && width > 0 && height > 0)) {
                m_pendingResize = true;
                m_pendingWidth  = width;
                m_pendingHeight = height;
                m_resizeTimer   = 0.0f;
            }

            if( m_pendingResize) {
                m_resizeTimer += _deltaTime;

                if( m_resizeTimer >= RESIZE_DEBOUNCE_SECONDS ) {
                    m_pendingResize = false;
                    m_resizeTimer = 0.0f;
                    m_windowHandle->setDimensions(width, height);
                    createSwapchain();
                }
            }

            if (!shouldRender()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                return;
            }

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            if (ImGui::Begin("background")) {

                auto& selected = m_backgroundEffects[m_currentBackgroundEffect];

                ImGui::Text("Selected effect: %s", selected.name.data());

                ImGui::SliderInt("Effect Index", &m_currentBackgroundEffect,0, static_cast< int >( m_backgroundEffects.size() - 1 ) );

                ImGui::ColorEdit4("Colour 1", reinterpret_cast<float*>(&selected.data.data1));
                ImGui::ColorEdit4("Colour 2", reinterpret_cast<float*>(&selected.data.data2));
                ImGui::ColorEdit4("Colour 3", reinterpret_cast<float*>(&selected.data.data3));
                ImGui::ColorEdit4("Colour 4", reinterpret_cast<float*>(&selected.data.data4));
            }
            ImGui::End();

            ImGui::Render();
            draw();
        }

        void draw() override {

            if( m_swapchain == VK_NULL_HANDLE ||
                m_windowHandle->dimension.width == 0 ||
                m_windowHandle->dimension.height == 0 ) return;

            vkUtil::vkCheck(
                vkWaitForFences( m_device
                               , 1
                               , &getCurrentFrame().m_inFlightFence
                               , VK_TRUE
                               , UINT64_MAX)
                               , "vkWaitForFences"
            );

            uint32_t imageIndex;
            VkResult acquireNextImageResult = vkAcquireNextImageKHR( m_device
                                                                   , m_swapchain
                                                                   , UINT64_MAX
                                                                   , getCurrentFrame().m_imageAvailableSemaphore
                                                                   , VK_NULL_HANDLE
                                                                   , &imageIndex
            );

            if( acquireNextImageResult == VK_ERROR_OUT_OF_DATE_KHR ||
                acquireNextImageResult == VK_SUBOPTIMAL_KHR ||
                acquireNextImageResult != VK_SUCCESS ) {
                createSwapchain();
                return;
            }

            // Reset fences now that we are proceeding
            vkUtil::vkCheck(
                vkResetFences( m_device
                    , 1
                    , &getCurrentFrame().m_inFlightFence )
                    , "resetFences"
            );

            getCurrentFrame().m_deletionQueue.flushDeletionQueue();

            VkCommandBuffer command = getCurrentFrame().commandBuffer;
            vkUtil::vkCheck(vkResetCommandBuffer( command, 0 ), "vkResetCommandBuffer");

            VkCommandBufferBeginInfo cmdBeginInfo = vkUtil::command_buffer_begin_info(
                VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

            // Begin commands
            vkUtil::vkCheck(vkBeginCommandBuffer( command, &cmdBeginInfo ), "vkBeginCommandBuffer");
            // some comments below to help me think better because im still not very good at this

            // Prepare draw image for shader
            vkUtil::transition_image( command
                                    , m_drawImage.image
                                    , VK_IMAGE_LAYOUT_UNDEFINED
                                    , VK_IMAGE_LAYOUT_GENERAL
            );

            drawBackground( command );

            // prepare image for copying FROM it
            vkUtil::transition_image( command
                                    , m_drawImage.image
                                    , VK_IMAGE_LAYOUT_GENERAL
                                    , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            );

            startGeometry( command );

            endGeometry( command );

            vkUtil::transition_image( command
                                    , m_drawImage.image
                                    , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                                    , VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
            );

            vkUtil::transition_image( command
                                    , m_swapchainImages[imageIndex]
                                    , VK_IMAGE_LAYOUT_UNDEFINED
                                    , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
            );

            // copy the draw image to swapchain image
            vkUtil::copy_image_to_image( command
                                       , m_drawImage.image
                                       , m_swapchainImages[imageIndex]
                                       , {m_drawImage.imageExtent.width, m_drawImage.imageExtent.height}
                                       , m_swapchainExtent
            );

            // prepare the swapchain for ImGui rendering
            vkUtil::transition_image( command
                                    , m_swapchainImages[imageIndex]
                                    , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                                    , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            );

            drawImGui( command, m_swapchainImageViews[ imageIndex ] );

            // prepare swapchain presentation
            vkUtil::transition_image( command
                                    , m_swapchainImages[imageIndex]
                                    , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                                    , VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
            );

            vkUtil::vkCheck(vkEndCommandBuffer( command ), "vkEndCommandBuffer");

            VkCommandBufferSubmitInfo cmdSubmitInfo = vkUtil::command_buffer_submit_info( command );

            VkSemaphoreSubmitInfo waitInfo = vkUtil::semaphore_submit_info(
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
                getCurrentFrame().m_imageAvailableSemaphore
            );

            // Use the class member semaphore here
            VkSemaphoreSubmitInfo signalInfo = vkUtil::semaphore_submit_info(
                VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                m_renderFinishedSemaphores[imageIndex]
            );

            VkSubmitInfo2 submitInfo = vkUtil::submit_info( &cmdSubmitInfo
                                                          , &signalInfo
                                                          , &waitInfo
            );

            vkUtil::vkCheck(
                vkQueueSubmit2( m_graphicsQueue
                              , 1
                              , &submitInfo
                              , getCurrentFrame().m_inFlightFence)
                              , "vkQueueSubmit2"
            );

            VkPresentInfoKHR presentInfo = { .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };

            presentInfo.pSwapchains        = &m_swapchain;
            presentInfo.swapchainCount     = 1;
            presentInfo.pWaitSemaphores    = &m_renderFinishedSemaphores[imageIndex];
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pImageIndices      = &imageIndex;

            VkResult presentResult = vkQueuePresentKHR(m_graphicsQueue, &presentInfo);

            if( presentResult == VK_ERROR_OUT_OF_DATE_KHR ||
                presentResult == VK_SUBOPTIMAL_KHR ||
                presentResult == VK_ERROR_SURFACE_LOST_KHR ) {
                createSwapchain();
            }

            m_frameNumber++;
        }

        void drawImGui( VkCommandBuffer _command, VkImageView _targetImageView ) {
            VkRenderingAttachmentInfoKHR colorAttachment = vkUtil::attachment_info( _targetImageView
                                                                                  , nullptr
                                                                                  , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            );

            VkRenderingInfo renderInfo = vkUtil::rendering_info( m_swapchainExtent
                                                               , &colorAttachment
                                                               , nullptr
            );

            vkCmdBeginRendering( _command, &renderInfo );

            ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(), _command );

            vkCmdEndRendering( _command );
        }

        void drawWithReflection(const void *_rawData, const sShaderReflection &_reflection) override {
            VkCommandBuffer cmd = getCurrentFrame().commandBuffer;
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_meshPipeline);

            for (const auto&[stageFlags, offset, size] : _reflection.pushConstants) {
                vkCmdPushConstants(
                    cmd,
                    m_meshPipelineLayout,
                    stageFlags,
                    offset,
                    size,
                    _rawData
                );
            }

            vkCmdBindIndexBuffer(cmd, m_rectangle.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);
        }

        void submitImmediate( std::function< void( VkCommandBuffer _command ) >&& _function ) {

            opn::logTrace("VulkanBackend", "Immediate command received.");

            vkUtil::vkCheck(
                vkResetFences( m_device, 1, &m_immediate.fence)
                             , "vkResetFences"
            );

            vkUtil::vkCheck(
                vkResetCommandBuffer( m_immediate.commandBuffer, 0 )
                                    , "vkResetCommandBuffer"
            );

            VkCommandBuffer command = m_immediate.commandBuffer;
            VkCommandBufferBeginInfo cmdBeginInfo = vkUtil::command_buffer_begin_info();

            vkUtil::vkCheck(
                vkBeginCommandBuffer( command, &cmdBeginInfo )
                                    , "vkBeginCommandBuffer"
            );

            opn::logTrace("VulkanBackend", "Submit command executing.");
            _function(command);
            opn::logTrace("VulkanBackend", "Submit command executed.");

            vkUtil::vkCheck(
                vkEndCommandBuffer( command )
                                  , "vkEndCommandBuffer"
            );

            VkCommandBufferSubmitInfo cmdSubmitInfo =
                vkUtil::command_buffer_submit_info( command );

            VkSubmitInfo2 submitInfo =
                vkUtil::submit_info( &cmdSubmitInfo, nullptr, nullptr );

            vkUtil::vkCheck(
                vkQueueSubmit2( m_graphicsQueue, 1, &submitInfo, m_immediate.fence )
                              , "vkQueueSubmit2"
            );

            vkUtil::vkCheck(
                vkWaitForFences( m_device, 1, &m_immediate.fence, true, 99999999 )
                               , "vkWaitForFences"
            );

            opn::logTrace("VulkanBackend", "Submit finished.");
        }

        [[nodiscard]] bool shouldRender() const {
            return m_swapchain != VK_NULL_HANDLE &&
                   m_windowHandle != nullptr &&
                   m_windowHandle->dimension.width > 0 &&
                   m_windowHandle->dimension.height > 0;
        }

        void drawBackground( VkCommandBuffer _command ) {
            if (m_backgroundEffects.empty())return;

            const auto& effect = m_backgroundEffects[m_currentBackgroundEffect];

            vkCmdBindPipeline( _command
                             , VK_PIPELINE_BIND_POINT_COMPUTE
                             , effect.pipeline
            );

            vkCmdBindDescriptorSets( _command
                                   , VK_PIPELINE_BIND_POINT_COMPUTE
                                   , effect.layout
                                   , 0, 1
                                   , &m_drawImageDescriptors
                                   , 0, nullptr
            );

            vkCmdPushConstants( _command
                              , effect.layout
                              , VK_SHADER_STAGE_COMPUTE_BIT
                              , 0
                              , sizeof( sComputePushConstants )
                              , &effect.data
            );

            vkCmdDispatch( _command
                         , static_cast< uint32_t >( std::ceil( m_drawImage.imageExtent.width / 16.0 ) )
                         , static_cast< uint32_t >( std::ceil( m_drawImage.imageExtent.height / 16.0 ) )
                         , 1
            );
        }

        void startGeometry( VkCommandBuffer _command ) {
            VkRenderingAttachmentInfo colourAttachment = vkUtil::attachment_info( m_drawImage.imageView
                                                                                  , nullptr
                                                                                  , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            );
            VkRenderingInfo renderInfo = vkUtil::rendering_info( m_drawImageExtent
                                                               , &colourAttachment
                                                               , nullptr
            );
            vkCmdBeginRendering( _command, &renderInfo );

            VkViewport viewport{};
            viewport.x = 0;
            viewport.y = 0;
            viewport.width = static_cast<float>(m_drawImage.imageExtent.width);
            viewport.height = static_cast<float>(m_drawImage.imageExtent.height);
            viewport.minDepth = 0.f;
            viewport.maxDepth = 1.f;
            vkCmdSetViewport( _command, 0, 1, &viewport );

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = m_drawImageExtent;
            vkCmdSetScissor( _command, 0, 1, &scissor );
        }

        void endGeometry( VkCommandBuffer _command ) {
            vkCmdEndRendering( _command );
        }

        void bindToWindow( WindowSurfaceProvider &_windowProvider ) override {
            opn::logDebug( "VulkanBackend", "Binding to window..." );

            m_windowHandle = &_windowProvider;
            m_surface = _windowProvider.createSurface( m_instance );

            if( !m_surface ) {
                opn::logCritical( "VulkanBackend", "Failed to create window surface!" );
                throw std::runtime_error( "Failed to create window surface!" );
            }

            completeInit();
        }
    };
}
