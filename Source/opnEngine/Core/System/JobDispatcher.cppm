module;

#include <functional>
#include <atomic>
#include <thread>
#include <utility>

export module opn.system.JobDispatcher;

import opn.utils.Thread.SPSCQueue;
import opn.Utils.Logging;

export namespace opn {
    enum class eJobType {
        General,
        Asset,
        Audio,
        Render,
    };

    struct sTask {
        std::function<void()> execute;
        uint32_t fenceID = 0;

        sTask() = default;
    };

    /**
     * @brief Monostate class.
     */
    class JobDispatcher {
        // private members
        inline static std::atomic_bool m_initialized{false};
        static constexpr size_t MAX_FENCES = 4096;
        static constexpr size_t QUEUE_SIZE = 1024;

    public:
        static void init() {
            if (m_initialized.exchange(true))
                throw std::runtime_error("JobDispatcher already initialized!");

            for (auto &f: s_fencePool) {
                f.store(0, std::memory_order_release);
            }
            logInfo("System", "Job Dispatching system initialized successfully.");

        }

        template<typename Func, typename... Args>
        static uint32_t Submit(const eJobType _type, Func &&_func, Args &&... _args) {
            const uint32_t fenceID = s_nextFenceID.fetch_add(1) % MAX_FENCES;
            s_fencePool[fenceID].store(1, std::memory_order_release);

            sTask newTask;

            newTask.fenceID = fenceID;
            newTask.execute = [function = std::forward<Func>(_func), ..._args = std::forward<Args>(_args)]() mutable {
                std::invoke(function, std::move(_args)...);
            };

            bool success = false;
            switch (_type) {
                case eJobType::General: success = s_generalQueue << std::move(newTask);
                    break;
                case eJobType::Asset: success = s_assetQueue << std::move(newTask);
                    break;
                case eJobType::Audio: success = s_audioQueue << std::move(newTask);
                    break;
                case eJobType::Render: success = s_renderQueue << std::move(newTask);
                    break;
            }
            if (!success) {
                s_fencePool[fenceID].store(0, std::memory_order_release);
                return 0;
            }
            return fenceID;
        }

        static bool isFenceSignaled(const uint32_t _fenceID) noexcept {
            return s_fencePool[_fenceID % MAX_FENCES].load(std::memory_order_acquire) == 0;
        }

        static void waitForFence(const uint32_t _fenceID) noexcept {
            s_fencePool[_fenceID % MAX_FENCES].wait(1, std::memory_order_acquire);
        }

        static SPSCQueue<sTask, QUEUE_SIZE> &getQueue(const eJobType _type) noexcept {
            switch (_type) {
                case eJobType::General: return s_generalQueue;
                case eJobType::Asset: return s_assetQueue;
                case eJobType::Audio: return s_audioQueue;
                case eJobType::Render: return s_renderQueue;
                default: return s_generalQueue;
            }
        }

        static void signalCompletion(const uint32_t _fenceID) noexcept {
            s_fencePool[_fenceID % MAX_FENCES].store(0, std::memory_order_release);
            s_fencePool[_fenceID % MAX_FENCES].notify_one();
        }

    private:
        inline static SPSCQueue<sTask, QUEUE_SIZE> s_generalQueue;
        inline static SPSCQueue<sTask, QUEUE_SIZE> s_assetQueue;
        inline static SPSCQueue<sTask, QUEUE_SIZE> s_audioQueue;
        inline static SPSCQueue<sTask, QUEUE_SIZE> s_renderQueue;

        inline static std::atomic<uint32_t> s_nextFenceID{0};
        inline static std::array<std::atomic<int32_t>, MAX_FENCES> s_fencePool{};
    };
}
