module;
#include <concepts>
#include <stdexcept>

export module opn.System.Service.Rendering;
import opn.Renderer.Backend;
import opn.System.ServiceInterface;
import opn.Utils.Logging;
import opn.System.Service.WindowSystem;

export namespace opn {
    template<typename T>
    concept ValidRenderer = std::derived_from<T, RenderBackend>;

    template<ValidRenderer Backend>
    class Rendering final : public Service<Rendering<Backend> > {
        Backend m_Backend{};

    protected:
        void onInit() override {
            m_Backend.init();
        };

        void onPostInit() override {
            if (auto *ws = WindowSystem::getService()) {
                m_Backend.bindToWindow(*ws);
            } else {
                opn::logCritical("Rendering", "Failed to bind rendering backend, window system not found." );
                throw std::runtime_error("Service dependency not satisfied: WindowSystem." );
            }
        };

        void onShutdown() override {
            m_Backend.shutdown();
        };

        void onUpdate(float _deltaTime) override {
            m_Backend.update(_deltaTime);
        };
    };
}
