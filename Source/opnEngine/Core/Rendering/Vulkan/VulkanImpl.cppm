module;
#include "vulkan/vulkan.hpp"
#include "VkBootstrap.h"

#include <atomic>

export module opn.Renderer.Vulkan;

import opn.Renderer.Backend;
import opn.System.WindowSurfaceProvider;
import opn.Rendering.Util.vkInit;
import opn.Rendering.Util.vkTypes;
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
            m_debugMessenger = m_vkbInstance.debug_messenger;

            opn::logInfo("VulkanBackend", "Vulkan instance created successfully.");
        }

        void createDevices() {
            if (!m_surface) {
                logCritical("VulkanBackend", "No surface provided to VulkanBackend!");
                throw std::runtime_error("No surface provided to VulkanBackend!");
            }

            VkPhysicalDeviceVulkan13Features features = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES
            };
            features.dynamicRendering = true;
            features.synchronization2 = true;

            VkPhysicalDeviceVulkan12Features features12 = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES
            };
            features12.bufferDeviceAddress = true;
            features12.descriptorIndexing = true;

            vkb::PhysicalDeviceSelector selector{m_vkbInstance};
            auto physicalDeviceRet = selector
                    .set_minimum_version(1, 3)
                    .set_required_features_13(features)
                    .set_required_features_12(features12)
                    .set_surface(m_surface)
                    .select();

            if (!physicalDeviceRet) {
                opn::logCritical("VulkanBackend", "Failed to find suitable GPU!");
                throw std::runtime_error("Failed to find suitable GPU!");
            }

            vkb::PhysicalDevice physicalDevice = physicalDeviceRet.value();

            vkb::DeviceBuilder deviceBuilder{physicalDevice};
            auto deviceRet = deviceBuilder.build();

            if (!deviceRet) {
                opn::logCritical("VulkanBackend", "Failed to create logical device!");
                throw std::runtime_error("Failed to create logical device!");
            }

            m_vkbDevice = deviceRet.value();
            m_device = m_vkbDevice.device;
            m_chosenDevice = physicalDevice.physical_device;

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

            for (int i = 0; i < FRAME_OVERLAP; i++) {
                vkT::vkCheck(
                    vkCreateCommandPool(m_device, &commandPoolInfo, nullptr, &m_frameData[i].commandPool),
                    "vkCreateCommandPool"
                );

                VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(
                    m_frameData[i].commandPool, 1
                );

                vkT::vkCheck(
                    vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &m_frameData[i].commandBuffer),
                    "vkAllocateCommandBuffers"
                );
            }
            opn::logInfo("VulkanBackend", "Command pools created.");
        };

        void createSyncObjects() {
            opn::logInfo("VulkanBackend", "Creating sync objects...");

            VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
            VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

            for (int i = 0; i < FRAME_OVERLAP; i++) {
                vkT::vkCheck(
                    vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_frameData[i].m_inFlightFence),
                    "vkCreateFence"
                );

                vkT::vkCheck(
                    vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_frameData[i].m_swapchainSemaphore),
                    "vkCreateSemaphore: swapchain"
                );
                vkT::vkCheck(
                    vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_frameData[i].m_renderSemaphore),
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

            opn::logInfo("VulkanBackend", "Initialization complete.");
        }

        void shutdown() final {
            logInfo("VulkanBackend", "Shutting down...");
            if (m_isInitialized.exchange(false)) {
                destroySwapchain();

                vkDeviceWaitIdle(m_device);
                for (int i = 0; i < FRAME_OVERLAP; ++i) {
                    vkDestroyCommandPool(m_device, m_frameData[i].commandPool, nullptr);
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
        }

        void draw() final {
            vkT::vkCheck(
                vkWaitForFences(m_device, 1, &getCurrentFrame().m_inFlightFence, true, 1000000000),
                "vkWaitForFences"
            );
            vkT::vkCheck(vkResetFences(m_device, 1, &getCurrentFrame().m_inFlightFence),
                         "vkResetFences"
            );

            uint32_t swapchainImageIndex;
            vkT::vkCheck(vkAcquireNextImageKHR(m_device, m_swapchain, 1000000000,
                                               getCurrentFrame().m_swapchainSemaphore, nullptr, &swapchainImageIndex),
                         "vkAcquireNextImageKHR"
            );

            VkCommandBuffer cmd = getCurrentFrame().commandBuffer;
            vkT::vkCheck(vkResetCommandBuffer(cmd, 0),
                         "vkResetCommandBuffer"
            );

            VkCommandBufferBeginInfo beginInfo = vkinit::command_buffer_begin_info(
                VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

            vkT::vkCheck(vkBeginCommandBuffer(cmd, &beginInfo),
                         "vkBeginCommandBuffer"
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
