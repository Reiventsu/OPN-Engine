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
        friend class VulkanImpl;
        friend class DirectXImpl;

    protected:
        void onInit() override {
            if constexpr (std::is_same_v<Backend, VulkanImpl>) {
                auto instance = Backend::initVulkan();


            }


            /* Will be overhauled later
            else if constexpr (std::is_same_v<Backend, DirectXImpl>)
                auto instance = Backend::template initializeRenderer();
            */
        };

        void onShutdown() override {
            if constexpr (std::is_same_v<Backend, VulkanImpl>)
                Backend::shutdownVulkan();
        };

        void onUpdate(float _deltaTime) override {

            if constexpr (std::is_same_v<Backend, VulkanImpl>) {

            }


            else if constexpr (std::is_same_v<Backend, DirectXImpl>) {

            }

        };
    };
}
