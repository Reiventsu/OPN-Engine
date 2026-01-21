module;
#include "vulkan/vulkan.hpp"
#include "VkBootstrap.h"

export module opn.Renderer.Vulkan;
import opn.Plugins.ThirdParty.hlslpp;
import opn.Utils.Logging;

export namespace opn {
    class VulkanImpl {
        VkInstance m_instance = nullptr;
        VkDebugUtilsMessengerEXT m_debugMessenger = nullptr;
        VkPhysicalDevice m_physicalDevice = nullptr;
        VkDevice m_device = nullptr;
        VkSurfaceKHR m_surface = nullptr;

        void initVulkan() {
            logInfo("VulkanBackend", "Initializing...");
            buildInstance();
        }

        void buildInstance() {
            vkb::InstanceBuilder builder;
            auto instance_return = builder
                    .set_app_name("OPN Engine")
                    .request_validation_layers()
                    .use_default_debug_messenger()
                    .build();

            vkb::Instance vkbInstance = instance_return.value;

            m_instance = vkbInstance.instance;
            m_debugMessenger = vkbInstance.debug_messenger;

            physicalDevice(instance_return);
        }

        void physicalDevice(auto &_instanceReturn) {
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

            vkb::PhysicalDeviceSelector selector = {_instanceReturn};
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
            m_physicalDevice = +physicalDevice.physical_device;
        }
    public:
        void bindBackend() {

        };
    };
}
