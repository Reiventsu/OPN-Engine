module;
#include <concepts>

export module opn.System.Service.Rendering;
import opn.System.ServiceInterface;
import opn.System.Service.WindowSystem;
import opn.Renderer.Vulkan;
import opn.Renderer.DirectX; // placeholder does nothing rn


export namespace opn {
    template<typename T>
    concept ValidRenderer = std::same_as<T, VulkanImpl> ||
                            std::same_as<T, DirectXImpl>;

    template<ValidRenderer Backend>
    class Rendering final : public Service<Rendering<Backend> > {
        Backend m_Backend{};

    protected:
        void onInit() override {
            if constexpr (std::is_same_v<Backend, VulkanImpl>) {
                m_Backend.initVulkan();
            } else if constexpr (std::is_same_v<Backend, DirectXImpl>)
                m_Backend.initDirectX();
        };

        void onShutdown() override {
            if constexpr (std::is_same_v<Backend, VulkanImpl>)
                m_Backend.shutdownVulkan();
            else if constexpr (std::is_same_v<Backend, DirectXImpl>)
                m_Backend.shutdownDirectX();
        };

        void onUpdate(float _deltaTime) override {
            if constexpr (std::is_same_v<Backend, VulkanImpl>) {
                // TODO update vulkan
            }
            else if constexpr (std::is_same_v<Backend, DirectXImpl>) {
                // TODO update DirectX
            }
        };
    };
}
