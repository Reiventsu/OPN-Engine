module;
#include "vulkan/vulkan.hpp"
#include "VkBootstrap.h"

export module opn.Renderer.Vulkan;
import opn.Plugins.ThirdParty.hlslpp;

using namespace hlslpp;

export namespace opn {
    class VulkanImpl {
        VkInstance m_instance = nullptr;
        VkDebugUtilsMessengerEXT m_debugMessenger = nullptr;
        VkPhysicalDevice m_physicalDevice = nullptr;
        VkDevice m_device = nullptr;
        VkSurfaceKHR m_surface = nullptr;


        std::optional<vkb::InstanceBuilder> buildInstance() {
            vkb::InstanceBuilder builder;
            auto instance = builder
                    .set_app_name("OPN Engine")
                    .request_validation_layers()
                    .use_default_debug_messenger()
                    .build();

            if (!instance)
                return std::nullopt;

            return builder;
        }

        void initializeRenderer() {
        };
    };
}
