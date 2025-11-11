#pragma once
#define GLFW_INCLUDE_VULKAN
#include <cstring>
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <print>

namespace {
    constexpr uint32_t windowWidth = 800;
    constexpr uint32_t windowHeight = 600;

    const std::vector<const char *> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
}

class VulkanRenderer {
public:

    void run();

private:

    // Private methods
    void initWindow();
    void createInstance();
    void initVulkan();
    void cleanup() const;
    void mainLoop() const;

    static bool checkValidationLayerSupport();

    static std::vector< const char* > getRequiredVulkanExtensions();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT _messageSeverity,
                                                         VkDebugUtilsMessageTypeFlagsEXT _messageType,
                                                         const VkDebugUtilsMessengerCallbackDataEXT* _pCallbackData,
                                                         void* _pUserData)
    {
        std::println( std::cerr, "Validation layer: {}", _pCallbackData->pMessage );

        return VK_FALSE;
    }

    // Private members
    GLFWwindow               *window        = nullptr;
    VkInstance               instance       = nullptr;
    VkDebugUtilsMessengerEXT debugMessenger = nullptr;
};
