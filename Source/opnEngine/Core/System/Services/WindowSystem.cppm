module;
#include <vulkan/vulkan.hpp>
#include "GLFW/glfw3.h"


export module opn.System.Service.WindowSystem;

import opn.System.ServiceInterface;
import opn.Utils.Logging;
import opn.System.WindowSurfaceProvider;

namespace opn {
    export class WindowSystem : public Service<WindowSystem>, public WindowSurfaceProvider {
        GLFWwindow *m_window = nullptr;
        int m_width = 1280;
        int m_height = 720;
        std::string m_title = "OPN Engine - Applicaiton";
        bool m_framebufferResized = false;

    public:
        [[nodiscard]] bool shouldClose() const {
            return m_window && glfwWindowShouldClose(m_window);
        }

        void pollEvents() const {
            glfwPollEvents();
        }

        void setTitle(const std::string &_title) {
            m_title = _title;
            if (!m_window) return;
            glfwSetWindowTitle(m_window, _title.c_str());
        }

        // Getters
        [[nodiscard]] GLFWwindow *getWindow() const { return m_window; }
        [[nodiscard]] int getWidth() const { return m_width; }
        [[nodiscard]] int getHeight() const { return m_height; }
        [[nodiscard]] bool wasResized() const { return m_framebufferResized; }
        void resetResizeFlag() { m_framebufferResized = false; }

        [[nodiscard]] VkSurfaceKHR createSurface(VkInstance _instance) const override {
            VkSurfaceKHR surface;
            if (glfwCreateWindowSurface(_instance, m_window, nullptr, &surface) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create window surface!");
            }
            return surface;
        }

        [[nodiscard]] GLFWwindow *getGLFWWindow() const override {
            return m_window;
        }

    protected:
        void onInit() override {
            logInfo("WindowSystem", "Initializing window system...");

            if (!glfwInit()) {
                throw std::runtime_error("Failed to initialize GLFW!");
            }

            if (!glfwVulkanSupported()) {
                glfwTerminate();
                throw std::runtime_error("Vulkan is not supported on this system!");
            }

            // Configure a window for Vulkan
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

            m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
            if (!m_window) {
                glfwTerminate();
                throw std::runtime_error("Failed to create GLFW window!");
            }

            setDimensions(m_width, m_height);

            // Set up callbacks
            glfwSetWindowUserPointer(m_window, this);
            glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);

            logInfo("WindowSystem", "Window created: {}x{}", m_width, m_height);
        }

        void onShutdown() override {
            logInfo("WindowSystem", "Shutting down window system...");

            if (m_window) {
                glfwDestroyWindow(m_window);
                m_window = nullptr;
            }
            glfwTerminate();

            logInfo("WindowSystem", "Window system shutdown successfully.");
        }

    private:
        static void framebufferResizeCallback(GLFWwindow *_window, int _width, int _height) {
            auto *ws = static_cast<WindowSystem *>(glfwGetWindowUserPointer(_window));
            ws->m_framebufferResized = true;
            ws->m_width = _width;
            ws->m_height = _height;
            // logDebug("WindowSystem", "Window resized: {}x{}", _width, _height);
        }

    public:
    };
}
