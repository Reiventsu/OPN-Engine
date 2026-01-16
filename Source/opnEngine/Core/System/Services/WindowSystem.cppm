module;
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

export module opn.System.Service.WindowSystem;

import opn.System.ServiceInterface;

namespace opn {
    class WindowSystem : public Service<WindowSystem> {
        GLFWwindow *m_window = nullptr;

    public:

    protected:
        void onInit() override {
            if (!glfwVulkanSupported())
                throw std::runtime_error("Vulkan is not supported on this system!");
        };

        void onShutdown() override {
        };
    };
}
