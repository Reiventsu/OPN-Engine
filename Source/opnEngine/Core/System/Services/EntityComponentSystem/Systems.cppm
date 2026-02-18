module;
#include <string>
#include "hlsl++.h"
export module opn.ECS:Systems;
import :Registry;
import opn.ECS.Components;
import opn.Utils.Logging;
import opn.Utils.Locator;
import opn.System.Service.Rendering;
import opn.System.Service.ShaderReflection;

export namespace opn::systems {
    class Systems final {
        friend class ECS;

        Registry* m_registry = nullptr;

    public:
        explicit Systems(Registry& _registry)
            : m_registry(&_registry) {}

        void renderMeshes() {
            auto& backend = Locator::getService<iRenderingService>()->getBackend();
            auto* compiler = Locator::getService<ShaderCompiler>();

            m_registry->forEach<components::Transform, components::Renderable>(
                [&](tEntity _entity, const auto& _transform, const auto& _renderable) {

                    std::string shaderPath = "shaders/default.slang";
                    if (m_registry->hasComponent<components::ShaderOverride>(_entity)) {
                    }

                    if (const auto reflection = compiler->compile(shaderPath); !reflection) {
                        opn::logError("ECS", "Failed to compile shader: {}", shaderPath);
                        return;
                    }

                }
            );
        }

        void rotateAll(float _deltaTime) {
            m_registry->forEach<components::Transform>(
                [_deltaTime](tEntity _entity, components::Transform& _transform) {
                    auto rotation = hlslpp::quaternion::rotation_axis(
                        hlslpp::float3(0, 1, 0),
                        _deltaTime * 0.5f
                    );
                    _transform.rotation = hlslpp::mul(_transform.rotation, rotation);
                }
            );
        }
    };
}
