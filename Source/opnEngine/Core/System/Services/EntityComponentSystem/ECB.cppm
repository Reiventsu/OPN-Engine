module;
#include <functional>
#include <mutex>
export module opn.ECS:ECB;
import :Registry;
import :tEntity;

namespace opn {
    class Registry;

    using EntityAction = std::move_only_function<void(Registry &)>;

    struct sECSCommand {
        enum class eType { Create, Destroy, AddComponent } type;

        tEntity entity;
        EntityAction action;

        sECSCommand(eType t, tEntity e, EntityAction &&act)
            : type(t), entity(e), action(std::move(act)) {
        }

        static sECSCommand Create(tEntity e, std::vector<tEntity> &tracking) {
            return {
                eType::Create, e, [&tracking, e](Registry &reg) {
                    reg.internal_registerCreate(e);
                    tracking.push_back(e);
                }
            };
        }

        template<typename T>
        static sECSCommand AddComponent(tEntity e, T &&component) {
            return {
                eType::AddComponent, e, [e, c = std::forward<T>(component)](Registry &reg) mutable {
                    if (reg.isValid(e)) {
                        reg.addComponent<T>(e, std::move(c));
                    }
                }
            };
        }

        static sECSCommand Destroy(tEntity e, std::vector<tEntity> &tracking) {
            return {
                eType::Destroy, e, [&tracking, e](Registry &reg) {
                    reg.internalDestroy(e);
                    std::erase(tracking, e);
                }
            };
        }
    };

    class EntityCommandBuffer {
        mutable std::vector<sECSCommand> m_commands;
                std::vector<sECSCommand> m_playbackBuffer;
        mutable std::mutex m_mutex;

    public:
        void enqueue(sECSCommand &&_command) const {
            std::scoped_lock lock(m_mutex);
            m_commands.emplace_back(std::move(_command));
        }

        void playback(Registry &_registry) {
            { // Swap magic for fast lock
                std::scoped_lock lock(m_mutex);
                m_playbackBuffer.swap(m_commands);
            }
            for (auto &command: m_playbackBuffer) {
                if (command.action) [[likely]] {
                    command.action(_registry);
                }
            }
            m_playbackBuffer.clear();
        }
    };
}
