module;
#include <volk.h>
#include <VkBootstrap.h>

#include <atomic>
#include <complex>
#include <deque>
#include <functional>

#include "vk_mem_alloc.h"

export module opn.Renderer.Vulkan;

import opn.Renderer.Backend;
import opn.System.WindowSurfaceProvider;
import opn.Rendering.Util.vk.vkInit;
import opn.Rendering.Util.vk.vkTypes;
import opn.Rendering.Util.vk.vkImage;
import opn.Utils.Logging;
import opn.Utils.Exceptions;

export namespace opn {
    class VulkanImpl : public RenderBackend {
        // Member variables
        std::atomic_bool             m_isInitialized{};
        uint32_t                     m_frameNumber{};
        const WindowSurfaceProvider *m_windowHandle = nullptr;

        VkInstance               m_instance = nullptr;
        VkDebugUtilsMessengerEXT m_debugMessenger = nullptr;
        VkPhysicalDevice         m_chosenDevice = nullptr;
        VkDevice                 m_device = nullptr;
        VkSurfaceKHR             m_surface = nullptr;
        VkSwapchainKHR           m_swapchain = nullptr;
        VkFormat                 m_swapchainImageFormat{};
        std::vector<VkImage>     m_swapchainImages;
        std::vector<VkImageView> m_swapchainImageViews;
        VkExtent2D               m_swapchainExtent = {};

        vkb::Instance      m_vkbInstance;
        vkb::Device        m_vkbDevice;
        vkb::DispatchTable m_dispatchTable;

        struct sDeletionQueue {
            std::deque<std::function<void()>> deletors;

            void pushFunction(std::function<void()>&& function) {
                deletors.emplace_back(function);
            }

            void flushDeletionQueue() {
                for (auto & deletor : deletors) {
                    deletor();
                }

                deletors.clear();
            }
        };

        struct sFrameData {
            VkSemaphore m_swapchainSemaphore{},
                        m_renderSemaphore{};
            VkFence     m_inFlightFence{};

            VkCommandPool   commandPool{};
            VkCommandBuffer commandBuffer{};

            sDeletionQueue m_deletionQueue;
        };

        struct sAllocatedImage {
            VkImage       image{};
            VkImageView   imageView{};
            VmaAllocation allocation{};
            VkExtent3D    imageExtent{};
            VkFormat      format{};
        };

        constexpr static uint8_t FRAME_OVERLAP = 2;
        sFrameData m_frameData[FRAME_OVERLAP];
        sFrameData &getCurrentFrame() { return m_frameData[m_frameNumber % FRAME_OVERLAP]; }

        sDeletionQueue m_mainDeletionQueue;
        VkQueue        m_graphicsQueue = nullptr;
        uint32_t       m_graphicsQueueFamily = 0;

        VmaAllocator m_vmaAllocator = nullptr;

        sAllocatedImage m_drawImage{};
        VkExtent2D      m_drawImageExtent{};


        void createInstance() {
            if (volkInitialize() != VK_SUCCESS) {
                throw std::runtime_error("Failed to initialize Volk!");
            }

            vkb::InstanceBuilder builder;
            auto instanceRet = builder
                    .set_app_name("OPN Engine")
                    .require_api_version( 1,3,0 )
                    .request_validation_layers()
                    .use_default_debug_messenger()
                    .build();

            if (!instanceRet)
                throw std::runtime_error("Failed to create Vulkan instance!");

            m_vkbInstance = instanceRet.value();
            m_instance = m_vkbInstance.instance;

            volkLoadInstance(m_instance);

            m_debugMessenger = m_vkbInstance.debug_messenger;

            VmaAllocatorCreateInfo allocatorCreateInfo{};
                                   allocatorCreateInfo.physicalDevice = m_chosenDevice;
                                   allocatorCreateInfo.device = m_device;
                                   allocatorCreateInfo.instance = m_instance;
                                   allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

            vmaCreateAllocator(&allocatorCreateInfo,&m_vmaAllocator);

            m_mainDeletionQueue.pushFunction( [ & ]() {
                vmaDestroyAllocator(m_vmaAllocator);
            });

            opn::logInfo("VulkanBackend", "Vulkan instance created successfully.");
        }

        void createDevices() {
            if (!m_surface) {
                logCritical("VulkanBackend", "No surface provided to VulkanBackend!");
                throw std::runtime_error("No surface provided to VulkanBackend!");
            }

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

            vkb::PhysicalDeviceSelector selector{m_vkbInstance};
            auto physicalDeviceRet = selector
                    .set_minimum_version(1, 3)
                    .set_required_features_13(features13)
                    .set_required_features_12(features12)
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

            opn::logInfo("VulkanBackend", "Vulkan device created successfully.");
        }

        void createSwapchain() {
            if (!m_windowHandle) {
                logCritical("VulkanBackend", "No window provided to VulkanBackend!");
                throw std::runtime_error("No window provided to VulkanBackend!");
            }


            vkb::SwapchainBuilder swapchainBuilder{m_chosenDevice, m_device, m_surface};

            m_swapchainImageFormat = VK_FORMAT_R8G8B8A8_UNORM;

            auto vkbSwapChainRet = swapchainBuilder
                    .set_desired_format(VkSurfaceFormatKHR{
                        .format = m_swapchainImageFormat,
                        .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
                    })
                    .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                    .set_desired_extent(m_windowHandle->dimension.width, m_windowHandle->dimension.height)
                    .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                    .build();

            if (!vkbSwapChainRet) {
                opn::logCritical("VulkanBackend", "Failed to create swapchain!");
                throw std::runtime_error("Failed to create swapchain!");
            }

            vkb::Swapchain vkbSwapChain = vkbSwapChainRet.value();
            m_swapchainExtent           = vkbSwapChain.extent;
            m_swapchain                 = vkbSwapChain.swapchain;
            m_swapchainImages           = vkbSwapChain.get_images().value();
            m_swapchainImageViews       = vkbSwapChain.get_image_views().value();

            logInfo("VulkanBackend", "Swapchain created: {}x{}.",
                    m_swapchainExtent.width, m_swapchainExtent.height);

            VkExtent3D drawImageExtent = {
                m_windowHandle->dimension.width,
                m_windowHandle->dimension.height,
                1
            };

            m_drawImage.format = VK_FORMAT_R16G16B16A16_SFLOAT;
            m_drawImage.imageExtent = drawImageExtent;

            VkImageUsageFlags drawImageUsages {};
            drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
            drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            VkImageCreateInfo rimgInfo = vkInit::image_create_info( m_drawImage.format
                                                                  , drawImageUsages
                                                                  , drawImageExtent
            );

            VmaAllocationCreateInfo rimgAllocInfo = {
                .usage = VMA_MEMORY_USAGE_GPU_ONLY,
                .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
            };

            vmaCreateImage(m_vmaAllocator
                          , &rimgInfo
                          , &rimgAllocInfo
                          , &m_drawImage.image
                          , &m_drawImage.allocation
                          , nullptr
            );

            VkImageViewCreateInfo rViewInfo = vkInit::imageview_create_info( m_drawImage.format
                                                                           , m_drawImage.image
                                                                           , VK_IMAGE_ASPECT_COLOR_BIT
            );

            vkUtil::vkCheck(vkCreateImageView( m_device
                                             , &rViewInfo
                                             , nullptr
                                             , &m_drawImage.imageView)
                                             , "vkCreateImageView"
            );

            m_mainDeletionQueue.pushFunction([=]() {
                vkDestroyImageView(m_device, m_drawImage.imageView, nullptr);
                vmaDestroyImage(m_vmaAllocator, m_drawImage.image, m_drawImage.allocation);
            });

        };

        void createCommands() {
            opn::logInfo("VulkanBackend", "Creating command pools...");
            VkCommandPoolCreateInfo commandPoolInfo =
                    vkInit::command_pool_create_info(m_graphicsQueueFamily
                                                     , VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

            for (auto &i: m_frameData) {
                vkUtil::vkCheck(
                    vkCreateCommandPool(m_device, &commandPoolInfo, nullptr, &i.commandPool),
                    "vkCreateCommandPool"
                );

                VkCommandBufferAllocateInfo cmdAllocInfo = vkInit::command_buffer_allocate_info(
                    i.commandPool, 1
                );

                vkUtil::vkCheck(
                    vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &i.commandBuffer),
                    "vkAllocateCommandBuffers"
                );
            }
            opn::logInfo("VulkanBackend", "Command pools created.");
        };

        void createSyncObjects() {
            opn::logInfo("VulkanBackend", "Creating sync objects...");

            VkFenceCreateInfo     fenceCreateInfo = vkInit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
            VkSemaphoreCreateInfo semaphoreCreateInfo = vkInit::semaphore_create_info();

            for (auto &i: m_frameData) {
                vkUtil::vkCheck(
                    vkCreateFence(m_device, &fenceCreateInfo, nullptr, &i.m_inFlightFence),
                    "vkCreateFence"
                );

                vkUtil::vkCheck(
                    vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &i.m_swapchainSemaphore),
                    "vkCreateSemaphore: swapchain"
                );
                vkUtil::vkCheck(
                    vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &i.m_renderSemaphore),
                    "vkCreateSemaphore: render"
                );
            }

            opn::logInfo("VulkanBackend", "Sync objects created.");
        }

        void destroySwapchain() {
            if (m_swapchain) {
                vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
                m_swapchain = nullptr;
            }

            for (const auto view: m_swapchainImageViews) {
                vkDestroyImageView(m_device, view, nullptr);
            }
            m_swapchainImageViews.clear();
            m_swapchainImages.clear();
        };

    public:
        void init() final {
            if (m_isInitialized.exchange(true))
                throw MultipleInit_Exception("VulkanBackend: Multiple init calls on graphics backend!");
            opn::logInfo("VulkanBackend", "Initializing...");
            createInstance();
        }

        void completeInit() {
            opn::logInfo("VulkanBackend", "Creating logical device...");
            createDevices();
            createSwapchain();

            opn::logInfo("VulkanBackend", "Creating queues...");
            m_graphicsQueue = m_vkbDevice.get_queue(vkb::QueueType::graphics).value();
            m_graphicsQueueFamily = m_vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

            createCommands();

            createSyncObjects();

            opn::logInfo("VulkanBackend", "Initialization complete.");
        }

        void shutdown() final {
            logInfo("VulkanBackend", "Shutting down...");
            if (m_isInitialized.exchange(false)) {
                destroySwapchain();

                vkDeviceWaitIdle(m_device);

                for (const auto &i: m_frameData) {
                    vkDestroyCommandPool(m_device, i.commandPool, nullptr);

                    vkDestroyFence(m_device, i.m_inFlightFence, nullptr);
                    vkDestroySemaphore(m_device, i.m_renderSemaphore, nullptr);
                    vkDestroySemaphore(m_device, i.m_swapchainSemaphore, nullptr);

                    m_frameData->m_deletionQueue.flushDeletionQueue();
                }
                m_mainDeletionQueue.flushDeletionQueue();

                vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
                vkDestroyDevice(m_device, nullptr);

                vkb::destroy_debug_utils_messenger(m_instance, m_debugMessenger);
                vkDestroyInstance(m_instance, nullptr);
            } else {
                logWarning("VulkanBackend", "Shutdown called, but backend was not initialized. Ignoring.");
            }
        }

        void update(float _deltaTime) final {
            if (m_isInitialized == false) return;

            draw();
        }

        void draw() final {
            if (m_swapchain == VK_NULL_HANDLE || m_swapchainImages.empty()) return

            vkUtil::vkCheck(
                vkWaitForFences(m_device, 1, &getCurrentFrame().m_inFlightFence, true, 1000000000),
                "vkWaitForFences"
            );

            getCurrentFrame().m_deletionQueue.flushDeletionQueue();

            vkUtil::vkCheck( vkResetFences( m_device, 1, &getCurrentFrame().m_inFlightFence ),
                            "vkResetFences"
            );

            uint32_t swapchainImageIndex;
            vkUtil::vkCheck(vkAcquireNextImageKHR( m_device
                                                 , m_swapchain
                                                 , 1000000000
                                                 , getCurrentFrame().m_swapchainSemaphore
                                                 , nullptr
                                                 , &swapchainImageIndex )
                                                 , "vkAcquireNextImageKHR"
            );

            //// Command buffer begin

            VkCommandBuffer cmd = getCurrentFrame().commandBuffer;
            vkUtil::vkCheck(vkResetCommandBuffer( cmd, 0 )
                                                , "vkResetCommandBuffer"
            );

            VkCommandBufferBeginInfo cmdBeginInfo = vkInit::command_buffer_begin_info(
                VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

            VkCommandBufferSubmitInfo cmdSubmitInfo = vkInit::command_buffer_submit_info(cmd);
            m_drawImageExtent.width = m_drawImage.imageExtent.width;
            m_drawImageExtent.height = m_drawImage.imageExtent.height;

            vkUtil::vkCheck( vkBeginCommandBuffer( cmd
                                                 , &cmdBeginInfo )
                                                 , "vkBeginCommandBuffer"
            );

            vkUtil::transition_image( cmd
                                    , m_drawImage.image
                                    , VK_IMAGE_LAYOUT_UNDEFINED
                                    , VK_IMAGE_LAYOUT_GENERAL
            );

            // Draw background here
            drawBackground(cmd);

            vkUtil::transition_image( cmd
                                    , m_drawImage.image
                                    , VK_IMAGE_LAYOUT_GENERAL
                                    , VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
            );

            vkUtil::transition_image( cmd
                                    , m_swapchainImages[ swapchainImageIndex ]
                                    , VK_IMAGE_LAYOUT_UNDEFINED
                                    , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
            );

            vkUtil::copy_image_to_image( cmd
                                       , m_drawImage.image
                                       , m_swapchainImages[ swapchainImageIndex ]
                                       , m_drawImageExtent
                                       , m_swapchainExtent
            );


            vkUtil::transition_image( cmd
                , m_swapchainImages[ swapchainImageIndex ]
                , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                , VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
            );

            vkUtil::vkCheck( vkEndCommandBuffer( cmd ), "vkEndCommandBuffer" );


            VkSemaphoreSubmitInfo waitInfo =
                vkInit::semaphore_submit_info( VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR
                                             , getCurrentFrame().m_swapchainSemaphore
                );
            VkSemaphoreSubmitInfo signalInfo =
                vkInit::semaphore_submit_info( VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT
                                             , getCurrentFrame().m_renderSemaphore
                );

            VkSubmitInfo2 submitInfo = vkInit::submit_info( &cmdSubmitInfo
                                                          , &signalInfo
                                                          , &waitInfo
            );

            vkUtil::vkCheck(vkQueueSubmit2(m_graphicsQueue
                                          , 1
                                          , &submitInfo
                                          , getCurrentFrame().m_inFlightFence)
                                          , ""
            );

            VkPresentInfoKHR presentInfo = {};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.pNext = nullptr;
            presentInfo.pSwapchains = &m_swapchain;
            presentInfo.swapchainCount = 1;

            presentInfo.pWaitSemaphores = &getCurrentFrame().m_renderSemaphore;
            presentInfo.waitSemaphoreCount = 1;

            presentInfo.pImageIndices = &swapchainImageIndex;

            vkUtil::vkCheck(vkQueuePresentKHR(m_graphicsQueue, &presentInfo), "");

            m_frameNumber++;

        }

        void drawBackground( VkCommandBuffer _cmd ) {
            VkClearColorValue clearValue;
            auto flash = static_cast<float>(std::abs( std::sin( m_frameNumber / 120.0 ) ));
            clearValue = { { 0.0f, 0.0f, flash, 1.0f } };

            VkImageSubresourceRange clearRange = vkInit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

            vkCmdClearColorImage(  _cmd
                                 , m_drawImage.image
                                 , VK_IMAGE_LAYOUT_GENERAL
                                 , &clearValue
                                 , 1
                                 , &clearRange
            );
        }

        void bindToWindow(const WindowSurfaceProvider &_windowProvider) final {
            logInfo("VulkanBackend", "Binding to window...");

            m_windowHandle = &_windowProvider;
            m_surface = _windowProvider.createSurface(m_instance);

            if (!m_surface) {
                opn::logCritical("VulkanBackend", "Failed to create window surface!");
                throw std::runtime_error("Failed to create window surface!");
            }

            completeInit();
        }

        void decoupleFromWindow();
    };
}
