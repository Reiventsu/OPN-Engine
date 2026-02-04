module;

#include <cstdint>
#include <memory>
#include <queue>
#include <ranges>
#include <typeindex>
#include <unordered_map>
#include <vector>

export module opn.ECS:Registry;

export namespace opn {
    class ECS;
    using tEntity = uint32_t;
    constexpr tEntity NULL_ENTITY = UINT32_MAX;

    namespace detail {
        class iComponentPool {
        public:
            virtual ~iComponentPool() = default;
            virtual void remove(tEntity _entity) = 0;
            [[nodiscard]] virtual bool has(tEntity _entity) const = 0;
        };

        template <typename T>
        class ComponentPool : public iComponentPool {
            friend class Registry;

            // data
            std::unordered_map<tEntity, size_t> m_entityIndex;
            std::vector<T> m_components;
            std::vector<tEntity> m_entities;

            // methods
            void insert(tEntity _entity, T _component) {
                if( m_entityIndex.contains(_entity)) {
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
                if (itr == m_entityIndex.end()) return;

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

            const T* get(tEntity _entity) const {
                auto itr = m_entityIndex.find(_entity);
                if (itr != m_entityIndex.end()) return nullptr;
                return &m_components[itr->second];
            }

            bool has(tEntity _entity) const override {
                return m_entityIndex.contains(_entity);
            }

            auto& components() {return m_components;}
            auto& entities() {return m_entities;}
            const auto& components() const {return m_components;}
            const auto& entities() const {return m_entities;}
        };
    }

    class Registry {
        friend class ECS;

        // data
        tEntity m_nextEntity{0};
        std::queue<tEntity> m_recycledEntities;
        std::unordered_map<std::type_index, std::unique_ptr<detail::iComponentPool>> m_componentPools;

        // methods
        template<typename T>
        detail::ComponentPool<T>* getPool() {
            auto typeIndex = std::type_index(typeid(T));

            if (!m_componentPools.contains(typeIndex)) {
                m_componentPools[typeIndex] = std::make_unique<detail::ComponentPool<T>>();
            }
            return static_cast<detail::ComponentPool<T>*>(m_componentPools[typeIndex].get());
        }

        tEntity create() {
            if (!m_recycledEntities.empty()) {
                tEntity entity = m_recycledEntities.front();
                m_recycledEntities.pop();
                return entity;
            }
            return m_nextEntity++;
        }

        void destroy(tEntity _entity) {
            for (auto &pool: m_componentPools | std::views::values) {
                pool->remove(_entity);
            }
            m_recycledEntities.push(_entity);
        }

        template<typename T>
        T& addComponent(tEntity _entity, T _component) {
            auto* pool = getPool<T>();
            pool->insert(_entity, std::move(_component));
            return *pool->get(_entity);
        }

        template<typename T>
        void removeComponent(tEntity _entity) {
            auto* pool = getPool<T>();
            pool->remove(_entity);
        }

        template<typename T>
        T* getComponent(tEntity _entity) {
            auto* pool = getPool<T>();
            return pool->get(_entity);
        }

        template<typename T>
        [[nodiscard]] bool hasComponent(tEntity _entity) const {
            auto typeIndex = std::type_index(typeid(T));
            if (!m_componentPools.contains(typeIndex)) return false;
            return m_componentPools.at(typeIndex)->has(_entity);
        }

        template<typename T, typename Func>
        void forEach(Func&& _func) {
            auto* pool = getPool<T>();
            auto& components = pool->components();
            auto& entities = pool->entities();

            for (size_t i = 0; i < components.size(); ++i) {
                _func(entities[i], components[i]);
            }
        }

        template<typename T1, typename T2, typename Func>
        void forEach(Func&& _func) {
            auto* pool1 = getPool<T1>();
            auto* pool2 = getPool<T2>();

            auto& entities1 = pool1->entities();
            auto& components1 = pool1->components();

            for (size_t i = 0; i < entities1.size(); ++i) {
                tEntity entity = entities1[i];
                if (auto* comp2 = pool2->get(entity)) {
                    _func(entity, components1[i], *comp2);
                }
            }
        }
    };
}
