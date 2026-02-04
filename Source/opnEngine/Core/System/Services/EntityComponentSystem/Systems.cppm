module;
#include "hlsl++.h"
export module opn.ECS:Systems;
import :Registry;
import opn.ECS.Components;
import opn.Utils.Logging;

export namespace opn::systems {
    class Systems final {
        friend class ECS;

        Registry* m_registry = nullptr;

    public:
        explicit Systems(Registry& _registry)
            : m_registry(&_registry) {}

        void renderMeshes() {
            m_registry->forEach<components::Transform, components::Renderable>(
                [](tEntity _entity,
                   const components::Transform& _transform,
                   const components::Renderable& _renderable) {

                    if (!_renderable.visible) return;

                    auto worldMatrix = _transform.getMatrix();

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