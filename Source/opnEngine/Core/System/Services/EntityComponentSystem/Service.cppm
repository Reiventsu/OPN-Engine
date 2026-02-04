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
        Registry m_registry;
        std::vector<tEntity> m_allEntities;

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
    };
}
