module;
#include <vector>
export module opn.ECS:Service;

import opn.System.ServiceInterface;
import :Registry;
import opn.ECS.Components;
import opn.Utils.Logging;

export namespace opn {
    class ECS final : public Service< ECS > {
        friend class Registry;

        // data
        Registry m_registry;
        std::vector<tEntity> m_allEntities;

        // service methods
    protected:
        void onInit() override {
        }

        void onShutdown() override {
            for (const auto entity : m_allEntities) {
                m_registry.destroy(entity);
            }
            m_allEntities.clear();
        }

        void onPostInit() override {
        }

        void onUpdate(const float _deltaTime) override {
        }

        // interaction methods
    public:
        tEntity createEntity() {
            return m_registry.create();
        }

        void destroyEntity(const tEntity _entity) {
            m_registry.destroy(_entity);
        }

        template<typename T>
        T& addComponent(tEntity _entity, T _component) {
            return m_registry.addComponent(_entity,_component);
        }

        template<typename T>
        void destroyComponent(tEntity _entity, T _component) {
            m_registry.removeComponent<_component>(_entity);
        }

    };
}
