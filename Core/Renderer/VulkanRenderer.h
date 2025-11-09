#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <print>

namespace {
    constexpr uint32_t windowWidth = 800;
    constexpr uint32_t windowHeight = 600;
}

class VulkanRenderer {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:

    // Private methods

    void initWindow();

    void createInstance();

    void initVulkan();
    void cleanup();
    void mainLoop();

    // Private memebers
    GLFWwindow* window;
    VkInstance instance;

};