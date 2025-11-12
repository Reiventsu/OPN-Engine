#pragma once
#define GLFW_INCLUDE_VULKAN
#include <cstring>
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <print>
#include <set>

namespace {
    constexpr uint32_t windowWidth = 800;
    constexpr uint32_t windowHeight = 600;

    const std::vector<const char *> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char *> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkResult CreateDebugUtilsMessengerEXT( VkInstance _instance,
                                           const VkDebugUtilsMessengerCreateInfoEXT* _pCreateInfo,
                                           const VkAllocationCallbacks* _pAllocator,
                                           VkDebugUtilsMessengerEXT* _pDebugMessenger)
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(_instance, _pCreateInfo, _pAllocator, _pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }
    void DestroyDebugUtilsMessengerEXT( VkInstance _instance,
                                        VkDebugUtilsMessengerEXT _debugMessenger,
                                        const VkAllocationCallbacks* _pAllocator)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(_instance, _debugMessenger, _pAllocator);
        }
    }
}

class VulkanRenderer {

    struct QueueFamilyIndices {
        std::optional< uint32_t > graphicsFamily;
        std::optional< uint32_t > presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() &&
                   presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };


public:

    void run();

private:

    // Private methods
    void createInstance();
    void initWindow();
    void initVulkan();
    void cleanup() const;
    void mainLoop() const;

    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSurface();
    void createSwapChain();
    void createImageViews();
    void createGraphicsPipeline();

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
    void setupDebugMessenger();

    QueueFamilyIndices findQueueFamilies( VkPhysicalDevice _device);
    SwapChainSupportDetails querySwapChainSupport( VkPhysicalDevice _device);

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

    bool isDeviceSuitable( VkPhysicalDevice _device );
    bool checkDeviceExtensionSupport( VkPhysicalDevice _device );
    static bool checkValidationLayerSupport();

    static std::vector< const char* > getRequiredVulkanExtensions();

    // Callback
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT _messageSeverity,
                                                         VkDebugUtilsMessageTypeFlagsEXT _messageType,
                                                         const VkDebugUtilsMessengerCallbackDataEXT* _pCallbackData,
                                                         void* _pUserData)
    {
        if ( _messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ) {
            std::println( std::cerr, "Validation layer: {}", _pCallbackData->pMessage );
        }
        return VK_FALSE;
    }

    // Private members

    VkDebugUtilsMessengerEXT debugMessenger = nullptr;

    GLFWwindow               *window        = nullptr;
    VkInstance               instance       = nullptr;

    VkPhysicalDevice         physicalDevice = VK_NULL_HANDLE;
    VkDevice                 device;
    VkSurfaceKHR             surface;

    VkSwapchainKHR           swapChain;
    std::vector<VkImage>     swapChainImages;
    VkFormat                 swapChainImageFormat;
    VkExtent2D               swapChainExtent;

    std::vector<VkImageView> swapChainImageViews;

    VkQueue                  graphicsQueue;
    VkQueue                  presentQueue;

};
