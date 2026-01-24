module;
#include <volk.h>
#include <VkBootstrap.h>

#include <atomic>
#include <complex>

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
        std::atomic_bool m_isInitialized{};
        uint32_t m_frameNumber{};
        const WindowSurfaceProvider *m_windowHandle = nullptr;

        VkInstance m_instance = nullptr;
        VkDebugUtilsMessengerEXT m_debugMessenger = nullptr;
        VkPhysicalDevice m_chosenDevice = nullptr;
        VkDevice m_device = nullptr;
        VkSurfaceKHR m_surface = nullptr;
        VkSwapchainKHR m_swapchain = nullptr;
        VkFormat m_swapchainImageFormat{};
        std::vector<VkImage> m_swapchainImages;
        std::vector<VkImageView> m_swapchainImageViews;
        VkExtent2D m_swapchainExtent = {};

        vkb::Instance m_vkbInstance;
        vkb::Device m_vkbDevice;
        vkb::DispatchTable m_dispatchTable;

        struct sFrameData {
            VkSemaphore m_swapchainSemaphore{}, m_renderSemaphore{};
            VkFence m_inFlightFence{};

            VkCommandPool commandPool{};
            VkCommandBuffer commandBuffer{};
        };

        constexpr static uint8_t FRAME_OVERLAP = 2;

        sFrameData m_frameData[FRAME_OVERLAP];
        sFrameData &getCurrentFrame() { return m_frameData[m_frameNumber % FRAME_OVERLAP]; }

        VkQueue m_graphicsQueue = nullptr;
        uint32_t m_graphicsQueueFamily = 0;


        void createInstance() {
            if (volkInitialize() != VK_SUCCESS) {
                throw std::runtime_error("Failed to initialize Volk!");
            }

            vkb::InstanceBuilder builder;
            auto instanceRet = builder
                    .set_app_name("OPN Engine")
                    .request_validation_layers()
                    .use_default_debug_messenger()
                    .build();

            if (!instanceRet)
                throw std::runtime_error("Failed to create Vulkan instance!");

            m_vkbInstance = instanceRet.value();
            m_instance = m_vkbInstance.instance;

            volkLoadInstance(m_instance);

            m_debugMessenger = m_vkbInstance.debug_messenger;
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
            features13.dynamicRendering = true;
            features13.synchronization2 = true;

            VkPhysicalDeviceVulkan12Features features12 = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES
            };
            features12.bufferDeviceAddress = true;
            features12.descriptorIndexing = true;

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

            m_dispatchTable = m_vkbDevice.make_table();
            volkLoadDevice(m_device);

            opn::logInfo("VulkanBackend", "Vulkan device created successfully.");
        }

        void createSwapchain() {
            if (!m_windowHandle) {
                logCritical("VulkanBackend", "No window provided to VulkanBackend!");
                throw std::runtime_error("No window provided to VulkanBackend!");
            }

            uint32_t width = m_windowHandle->dimension.width;
            uint32_t height = m_windowHandle->dimension.height;

            if (width == 0 || height == 0) return;



            vkb::SwapchainBuilder swapchainBuilder{m_chosenDevice, m_device, m_surface};

            m_swapchainImageFormat = VK_FORMAT_R8G8B8A8_UNORM;

            auto vkbSwapChainRet = swapchainBuilder
                    .set_desired_format(VkSurfaceFormatKHR{
                        .format = m_swapchainImageFormat,
                        .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
                    })
                    .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                    .set_desired_extent(width, height)
                    .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                    .build();

            if (!vkbSwapChainRet) {
                opn::logCritical("VulkanBackend", "Failed to create swapchain!");
                throw std::runtime_error("Failed to create swapchain!");
            }

            vkb::Swapchain vkbSwapChain = vkbSwapChainRet.value();
            m_swapchainExtent = vkbSwapChain.extent;
            m_swapchain = vkbSwapChain.swapchain;
            m_swapchainImages = vkbSwapChain.get_images().value();
            m_swapchainImageViews = vkbSwapChain.get_image_views().value();

            logInfo("VulkanBackend", "Swapchain created: {}x{}.",
                    m_swapchainExtent.width, m_swapchainExtent.height);
        };

        void createCommands() {
            opn::logInfo("VulkanBackend", "Creating command pools...");
            VkCommandPoolCreateInfo commandPoolInfo =
                    vkinit::command_pool_create_info(m_graphicsQueueFamily
                                                     , VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

            for (auto &i: m_frameData) {
                vkUtil::vkCheck(
                    vkCreateCommandPool(m_device, &commandPoolInfo, nullptr, &i.commandPool),
                    "vkCreateCommandPool"
                );

                VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(
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

            VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
            VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

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
                }

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
            if (m_swapchain == VK_NULL_HANDLE || m_swapchainImages.empty()) return;

            vkUtil::vkCheck(
                vkWaitForFences(m_device, 1, &getCurrentFrame().m_inFlightFence, true, 1000000000),
                "vkWaitForFences"
            );
            vkUtil::vkCheck( vkResetFences( m_device, 1, &getCurrentFrame().m_inFlightFence ),
                            "vkResetFences"
            );

            uint32_t swapchainImageIndex;
            vkUtil::vkCheck(vkAcquireNextImageKHR(m_device, m_swapchain, 1000000000,
                                                  getCurrentFrame().m_swapchainSemaphore, nullptr,
                                                  &swapchainImageIndex),
                            "vkAcquireNextImageKHR"
            );

            VkCommandBuffer cmd = getCurrentFrame().commandBuffer;
            vkUtil::vkCheck(vkResetCommandBuffer(cmd, 0),
                            "vkResetCommandBuffer"
            );

            VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(
                VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

            vkUtil::vkCheck(vkBeginCommandBuffer(cmd, &cmdBeginInfo),
                            "vkBeginCommandBuffer"
            );

            vkUtil::transition_image( m_dispatchTable.fp_vkCmdPipelineBarrier2
                                    , cmd
                                    , m_swapchainImages[swapchainImageIndex]
                                    , VK_IMAGE_LAYOUT_UNDEFINED
                                    , VK_IMAGE_LAYOUT_GENERAL
            );

            VkClearColorValue clearValue;
            float flash = std::abs( std::sin( m_frameNumber / 120 ) );
            clearValue = { { 0.0f, 0.0f, flash, 1.0f } };

            VkImageSubresourceRange clearRange = vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

            vkCmdClearColorImage(  cmd
                                 , m_swapchainImages[swapchainImageIndex]
                                 , VK_IMAGE_LAYOUT_GENERAL
                                 , &clearValue
                                 , 1
                                 , &clearRange
            );

            vkUtil::transition_image( m_dispatchTable.fp_vkCmdPipelineBarrier2
                                    , cmd
                                    , m_swapchainImages[swapchainImageIndex]
                                    , VK_IMAGE_LAYOUT_GENERAL
                                    , VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
            );

            vkUtil::vkCheck( vkEndCommandBuffer(cmd), "vkEndCommandBuffer" );

            VkCommandBufferSubmitInfo cmdSubmitInfo = vkinit::command_buffer_submit_info(cmd);

            VkSemaphoreSubmitInfo waitInfo =
                vkinit::semaphore_submit_info( VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR
                                             , getCurrentFrame().m_swapchainSemaphore
                );
            VkSemaphoreSubmitInfo signalInfo =
                vkinit::semaphore_submit_info( VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT
                                             , getCurrentFrame().m_renderSemaphore
                );

            VkSubmitInfo2 submitInfo = vkinit::submit_info( &cmdSubmitInfo
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
