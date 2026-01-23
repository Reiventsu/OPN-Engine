module;
#include "vulkan/vulkan.hpp"
#include "VkBootstrap.h"

#include <atomic>

export module opn.Renderer.Vulkan;

import opn.Renderer.Backend;
import opn.System.WindowSurfaceProvider;
import opn.Plugins.ThirdParty.hlslpp;
import opn.Utils.Logging;
import opn.Utils.Exceptions;

export namespace opn {
    class VulkanImpl : public RenderBackend {
        std::atomic_bool m_isInitialized{};

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

        const WindowSurfaceProvider *m_windowHandle = nullptr;

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

            vkb::Device vkbDevice = deviceRet.value();
            m_device = vkbDevice.device;
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
            opn::logInfo("VulkanBackend", "Initialization complete.");
        }

        void shutdown() final {
            logInfo("VulkanBackend", "Shutting down...");
            if (m_isInitialized.exchange(false)) {
                destroySwapchain();

                vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
                vkDestroyDevice(m_device, nullptr);

                vkb::destroy_debug_utils_messenger(m_instance, m_debugMessenger);
                vkDestroyInstance(m_instance, nullptr);
            }
        }

        void update(float _deltaTime) final {
        }

        void render() final {
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
