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


        const WindowSurfaceProvider *m_windowHandle = nullptr;

        void buildBackend() {
            vkb::InstanceBuilder builder;
            auto instance_return = builder
                    .set_app_name("OPN Engine")
                    .request_validation_layers()
                    .use_default_debug_messenger()
                    .build();

            vkb::Instance vkbInstance = instance_return.value();

            m_instance = vkbInstance.instance;
            m_debugMessenger = vkbInstance.debug_messenger;

            physicalDevice(vkbInstance);
        }

        void physicalDevice(vkb::Instance &_vkbInst) {
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

            vkb::PhysicalDeviceSelector selector{_vkbInst};
            vkb::PhysicalDevice physicalDevice = selector
                    .set_minimum_version(1, 3)
                    .set_required_features_13(features)
                    .set_required_features_12(features12)
                    .set_surface(m_surface)
                    .select()
                    .value();

            vkb::DeviceBuilder deviceBuilder{physicalDevice};
            vkb::Device vkbDevice = deviceBuilder.build().value();

            m_device = vkbDevice.device;
            m_chosenDevice = +physicalDevice.physical_device;
        }

        void createSwapchain() {
            vkb::SwapchainBuilder swapchainBuilder{m_chosenDevice, m_device, m_surface};

            m_swapchainImageFormat = VK_FORMAT_R8G8B8A8_UNORM;

            vkb::Swapchain vkbSwapChain = swapchainBuilder
                    .set_desired_format(VkSurfaceFormatKHR{
                        .format = m_swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
                    })
                    .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                    .set_desired_extent(m_windowHandle->dimension.height, m_windowHandle->dimension.width)
                    .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                    .build()
                    .value();

            m_swapchainExtent = vkbSwapChain.extent;
            m_swapchain = vkbSwapChain.swapchain;
            m_swapchainImages = vkbSwapChain.get_images().value();
            m_swapchainImageViews = vkbSwapChain.get_image_views().value();
        };

        void destroySwapchain() {
            vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

            for (int i = 0; i < m_swapchainImageViews.size(); i++) {
                vkDestroyImageView(m_device, m_swapchainImageViews[i], nullptr);
            }
        };

        //// Bootstrapping callers.
    public:
        void init() final {
            if (m_isInitialized.exchange(true))
                throw MultipleInit_Exception("VulkanBackend: Multiple init calls on graphics backend!");
            logInfo("VulkanBackend", "Initializing...");
            buildBackend();
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
            m_windowHandle = &_windowProvider;
            m_surface = _windowProvider.createSurface(m_instance);
        };
        void decoupleFromWindow();
    };
}
