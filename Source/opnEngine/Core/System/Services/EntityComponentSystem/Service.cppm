module;
#include <vector>
export module opn.ECS:Service;

import opn.System.ServiceInterface;
import :Registry;
import :Systems;  // ✅ Import Systems
import opn.ECS.Components;
import opn.Utils.Logging;
import opn.System.JobDispatcher;

export namespace opn {
    class EntityComponentSystem final : public Service<EntityComponentSystem> {
        // Data
        Registry m_registry;
        systems::Systems m_systems{m_registry};
        std::vector<tEntity> m_allEntities;

    protected:
        void onInit() override {
            logInfo("ECS", "Entity Component System initialized.");
        }

        void onShutdown() override {
            logInfo("ECS", "Shutting down ECS...");

            for (const auto entity : m_allEntities) {
                m_registry.destroy(entity);
            }
            m_allEntities.clear();

            logInfo("ECS", "ECS shutdown complete.");
        }

        void onPostInit() override {
            logInfo("ECS", "ECS post-initialization complete.");
        }

        void onUpdate(const float _deltaTime) override {
            m_systems.rotateAll(_deltaTime);

        }

    public:

        tEntity createEntity() {
            tEntity e = m_registry.create();
            m_allEntities.push_back(e);
            logTrace("ECS", "Created entity: {}", e);
            return e;
        }

        void destroyEntity(const tEntity _entity) {
            logTrace("ECS", "Destroying entity: {}", _entity);
            m_registry.destroy(_entity);

            // Remove from tracking
            std::erase(m_allEntities, _entity);
        }

        template<typename T>
        T& addComponent(tEntity _entity, T _component) {
            return m_registry.addComponent(_entity, std::move(_component));
        }

        template<typename T>
        void removeComponent(tEntity _entity) {
            m_registry.removeComponent<T>(_entity);
        }

        template<typename T>
        T* getComponent(tEntity _entity) {
            return m_registry.getComponent<T>(_entity);
        }

        template<typename T>
        bool hasComponent(tEntity _entity) const {
            return m_registry.hasComponent<T>(_entity);
        }

        void executeRenderSystems() {
            m_systems.renderMeshes();
        }


        [[nodiscard]] size_t getEntityCount() const noexcept {
            return m_allEntities.size();
        }

        [[nodiscard]] bool isEntityValid(tEntity _entity) const noexcept {
            return std::ranges::find(m_allEntities, _entity) != m_allEntities.end();
        }
    };
}