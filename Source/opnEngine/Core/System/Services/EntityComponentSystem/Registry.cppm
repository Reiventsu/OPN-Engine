module;

#include <cstdint>
#include <unordered_map>
#include <vector>

export module opn.Systems.ECS.Registry;

export namespace opn {
    using tEntity = uint32_t;
    constexpr tEntity NULL_ENTITY = UINT32_MAX;

    namespace detail {
        class iComponentPool {
            public:
            virtual ~iComponentPool() = default;
            virtual void remove(tEntity _entity) = 0;
            virtual bool has(tEntity _entity) const = 0;
        };

        template <typename T>
        class ComponentPool : public iComponentPool {
            std::unordered_map<tEntity, size_t> m_entityIndex;
            std::vector<T> m_components;
            std::vector<tEntity> m_entities;
            public:
            void insert(tEntity _entity, T _component) {
                if ( m_entityIndex.contains(_entity)) {
                    m_components[m_entityIndex[_entity]] = std::move(_component);
                    return;
                }
                size_t index = m_components.size();
                m_components.push_back(std::move(_component));
                m_entities.push_back(_entity);
                m_entityIndex[_entity] = index;
            }

            void remove(tEntity _entity) override {
                auto itr = m_entityIndex.find(_entity);
                if (itr != m_entityIndex.end()) return;

                size_t index = itr->second;
                size_t lastIndex = m_entities.size() - 1;

                if (index != lastIndex) {
                    m_components[index] = std::move(m_components[lastIndex]);
                    m_entities[index] = m_entities[lastIndex];
                    m_entityIndex[m_entities[index]] = index;
                }

                m_components.pop_back();
                m_entities.pop_back();
                m_entityIndex.erase(itr);
            }

            T* get(tEntity _entity) {
                auto itr = m_entityIndex.find(_entity);
                if (itr != m_entityIndex.end()) return nullptr;
                return &m_components[itr->second];
            }

            bool has(tEntity _entity) const override {
                return m_entityIndex.contains(_entity);
            }
        };
    }
}
