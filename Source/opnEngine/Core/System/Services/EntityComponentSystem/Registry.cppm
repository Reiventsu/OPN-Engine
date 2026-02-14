module;

#include <atomic>
#include <cstdint>
#include <memory>
#include <queue>
#include <ranges>
#include <typeindex>
#include <unordered_map>
#include <vector>

export module opn.ECS:Registry;
import :tEntity;

export namespace opn {
    class EntityComponentSystem;

    namespace systems {
        class Systems;
    }

    namespace detail {
        class iComponentPool {
        public:
            virtual ~iComponentPool() = default;

            virtual void remove(tEntity _entity) = 0;

            [[nodiscard]] virtual bool has(tEntity _entity) const = 0;
        };

        template<typename T>
        class ComponentPool : public iComponentPool {
            friend class Registry;

            std::vector<uint32_t> m_sparse;
            std::vector<T> m_components;
            std::vector<tEntity> m_entities;

        public:
            void insert(tEntity _entity, T _component) {
                uint32_t id = _entity.index();

                if (id >= m_sparse.size()) m_sparse.resize(id + 1, 0xFFFFFFFF);

                if (m_sparse[id] != 0xFFFFFFFF) {
                    m_components[m_sparse[id]] = std::move(_component);
                    return;
                }

                m_sparse[id] = static_cast<uint32_t>(m_entities.size());
                m_entities.push_back(_entity);
                m_components.push_back(std::move(_component));
            }

            void remove(tEntity _entity) override {
                uint32_t id = _entity.index();
                if (id >= m_sparse.size() || m_sparse[id] == 0xFFFFFFF) return;

                uint32_t indexToRemove = m_sparse[id];
                uint32_t lastIndex = static_cast<uint32_t>(m_entities.size() - 1);
                tEntity lastEntity = m_entities[lastIndex];

                if (indexToRemove != lastIndex) {
                    m_components[indexToRemove] = std::move(m_components[lastIndex]);
                    m_entities[indexToRemove] = m_entities[lastIndex];

                    m_sparse[lastEntity.index()] = indexToRemove;
                }

                m_components.pop_back();
                m_entities.pop_back();
                m_sparse[id] = 0xFFFFFFFF;
            }

            T *get(const tEntity _entity) {
                const uint32_t id = _entity.index();
                if (id >= m_sparse.size() || m_sparse[id] == 0xFFFFFFFF) return nullptr;
                return &m_components[m_sparse[id]];
            }

            const T *get(const tEntity _entity) const {
                const uint32_t id = _entity.index();
                if (id >= m_sparse.size() || m_sparse[id] == 0xFFFFFFFF) return nullptr;
                return &m_components[m_sparse[id]];
            }

            [[nodiscard]] bool has(tEntity _entity) const override {
                const uint32_t id = _entity.index();
                return id < m_sparse.size() && m_sparse[id] != 0xFFFFFFFF;
            }

            auto &components() { return m_components; }
            auto &entities() { return m_entities; }
            const auto &components() const { return m_components; }
            const auto &entities() const { return m_entities; }
        };
    }

    class Registry final {
        friend class EntityComponentSystem;
        friend class systems::Systems;
        friend class EntityCommandBuffer;
        friend struct sECSCommand;

        std::vector<uint32_t> m_generations;
        mutable std::atomic<uint32_t> m_indexCounter{0};

        std::unordered_map<std::type_index, std::unique_ptr<detail::iComponentPool> > m_componentPools;

        template<typename T>
        detail::ComponentPool<T> *getPool() {
            auto typeIndex = std::type_index(typeid(T));

            if (!m_componentPools.contains(typeIndex)) {
                m_componentPools[typeIndex] = std::make_unique<detail::ComponentPool<T> >();
            }
            return static_cast<detail::ComponentPool<T> *>(m_componentPools[typeIndex].get());
        }

        [[nodiscard]] bool isValid(const tEntity _entity) const noexcept {
            const uint32_t idx = _entity.index();

            if (idx >= m_generations.size()) return false;
            return m_generations[idx] == _entity.generation();
        }

        tEntity create() const {
            const uint32_t idx = m_indexCounter.fetch_add(1, std::memory_order_relaxed);
            return tEntity::make(idx, 1);
        }

        void internal_registerCreate(tEntity _entity) {
            const uint32_t idx = _entity.index();
            if (idx >= m_generations.size())
                m_generations.resize(idx + 1024, 0);
        }

        void internalDestroy(tEntity _entity) {
            if (!isValid(_entity)) return;

            const uint32_t idx = _entity.index();
            if (m_generations[idx] != _entity.generation()) return;
            for (const auto &pool: m_componentPools | std::views::values) {
                pool->remove(_entity);
            }

            m_generations[idx]++;
        }

        template<typename T>
        T &addComponent(tEntity _entity, T _component) {
            auto *pool = getPool<T>();
            pool->insert(_entity, std::move(_component));
            return *pool->get(_entity);
        }

        template<typename T>
        void removeComponent(tEntity _entity) {
            auto *pool = getPool<T>();
            pool->remove(_entity);
        }

        template<typename T>
        T *getComponent(tEntity _entity) {
            auto *pool = getPool<T>();
            return pool->get(_entity);
        }

        template<typename T>
        [[nodiscard]] bool hasComponent(tEntity _entity) const {
            auto typeIndex = std::type_index(typeid(T));
            if (!m_componentPools.contains(typeIndex)) return false;
            return m_componentPools.at(typeIndex)->has(_entity);
        }

        template<typename T, typename Func>
        void forEach(Func &&_func) {
            auto *pool = getPool<T>();
            auto &components = pool->components();
            auto &entities = pool->entities();

            for (size_t i = 0; i < components.size(); ++i) {
                _func(entities[i], components[i]);
            }
        }

        template<typename T1, typename T2, typename Func>
        void forEach(Func &&_func) {
            auto *pool1 = getPool<T1>();
            auto *pool2 = getPool<T2>();

            auto &entities1 = pool1->entities();
            auto &components1 = pool1->components();

            for (size_t i = 0; i < entities1.size(); ++i) {
                tEntity entity = entities1[i];
                if (auto *comp2 = pool2->get(entity)) {
                    _func(entity, components1[i], *comp2);
                }
            }
        }
    };
}
