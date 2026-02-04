module;

#include <array>
#include <functional>
#include <atomic>
#include <source_location>
#include <thread>
#include <utility>
#include <vector>
#include <unordered_map>
#include <mutex>

#include <bit>

export module opn.System.JobDispatcher;

import opn.System.Thread.MPSCQueue;
import opn.Utils.Logging;
import opn.Utils.Exceptions;

export namespace opn {
    enum class eJobType {
        General,
        Asset,
        Audio,
        Render,
        COUNT
    };

    struct sTask {
        std::move_only_function<void()> execute;
        uint32_t fenceID = 0;

        sTask() = default;
        sTask(sTask&& other) noexcept :
            execute(std::move(other.execute)),
            fenceID(other.fenceID) {}

        sTask& operator=(sTask&& other) noexcept {
            execute = std::move(other.execute);
            fenceID = other.fenceID;
            return *this;
        }
    };

    struct sJobHandle;

    /**
     * @brief Monostate class for Job System.
     */
    class JobDispatcher {
        struct sDeferredJob {
            eJobType type;
            sTask task;
        };

        // private members
        inline static std::atomic_bool initialized{false};
        static constexpr size_t MAX_FENCES = 4096;
        static constexpr size_t QUEUE_SIZE = 1024;

        inline static std::mutex s_dependencyMutex;
        inline static std::unordered_map<uint32_t, std::vector<sDeferredJob> > s_dependencies;

        inline static std::array<std::atomic<uint32_t>, static_cast<size_t>(eJobType::COUNT)> s_queueSignals{};

        inline static MPSCQueue<sTask, QUEUE_SIZE> s_generalQueue;
        inline static MPSCQueue<sTask, QUEUE_SIZE> s_assetQueue;
        inline static MPSCQueue<sTask, QUEUE_SIZE> s_audioQueue;
        inline static MPSCQueue<sTask, QUEUE_SIZE> s_renderQueue;

        inline static std::atomic<uint32_t> s_nextFenceID{0};
        inline static std::array<std::atomic<int32_t>, MAX_FENCES> s_fencePool{};

    public:
        static void init(const std::source_location loc = std::source_location::current()) {
            if (initialized.exchange(true, std::memory_order_acq_rel))
                throw MultipleInit_Exception("JobDispatcher", loc);

            for (auto &f: s_fencePool) f.store(0, std::memory_order_release);
            opn::logInfo("JobDispatcher", "Job Dispatcher initialized successfully.");
        }

        static void shutdown() {
            opn::logInfo("JobDispatcher", "Shutting down Job Dispatching service...");
            if (!initialized.exchange(false)) return;

            for (size_t i = 0; i < static_cast<size_t>(eJobType::COUNT); ++i)
                wakeWorkers(static_cast<eJobType>(i));
            s_nextFenceID.store(0);
        }

        // Templates defined below
        template<typename Command>
        static sJobHandle submit(eJobType _type, Command &&_command);

        template<typename Command>
        static sJobHandle submitAfter(uint32_t _dependencyFence, eJobType _type, Command &&_command);

        static bool isFenceSignaled(const uint32_t _fenceID) noexcept {
            return s_fencePool[_fenceID % MAX_FENCES].load(std::memory_order_acquire) == 0;
        }

        static void waitForFence(const uint32_t _fenceID) noexcept {
            s_fencePool[_fenceID % MAX_FENCES].wait(1, std::memory_order_acquire);
        }

        static void waitForWork(const eJobType _type, uint32_t &_lastSeenSignal) noexcept {
            const auto &queue = getQueue(_type);
            const auto &signal = s_queueSignals[static_cast<size_t>(_type)];

            if (queue.isEmpty()) {
                signal.wait(_lastSeenSignal, std::memory_order_acquire);
            }
            _lastSeenSignal = signal.load(std::memory_order_acquire);
        }

        static void wakeWorkers(const eJobType _type) noexcept {
            auto &signal = s_queueSignals[static_cast<size_t>(_type)];
            signal.fetch_add(1, std::memory_order_release);
            signal.notify_all();
        }

        static MPSCQueue<sTask, QUEUE_SIZE> &getQueue(const eJobType _type) noexcept {
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
            s_fencePool[_fenceID % MAX_FENCES].notify_all();

            {
                std::lock_guard lock(s_dependencyMutex);
                if (const auto itr = s_dependencies.find(_fenceID); itr != s_dependencies.end()) {
                    for (auto &[type, task]: itr->second) {
                        dispatchInternal(type, std::move(task));
                    }
                    s_dependencies.erase(itr);
                }
            }
        }

    private:
        static void dispatchInternal(eJobType _type, sTask &&_task) {
            bool success = false;
            switch (_type) {
                case eJobType::General: success = s_generalQueue << std::move(_task);
                    break;
                case eJobType::Asset: success = s_assetQueue << std::move(_task);
                    break;
                case eJobType::Audio: success = s_audioQueue << std::move(_task);
                    break;
                case eJobType::Render: success = s_renderQueue << std::move(_task);
                    break;
                default: break;
            }
            if (success) {
                auto &signal = s_queueSignals[static_cast<size_t>(_type)];
                signal.fetch_add(1, std::memory_order_release);
                signal.notify_one();
            } else {
                logError("JobDispatcher", "Failed to dispatch deferred task!");
            }
        }
    };

    // --- Job Handle Definition ---
    struct sJobHandle {
        uint32_t fenceID{};

        void wait() const noexcept { JobDispatcher::waitForFence(fenceID); }
        bool isReady() const noexcept { return JobDispatcher::isFenceSignaled(fenceID); }

        template<typename Command>
        sJobHandle then(const eJobType _nextType, Command &&_nextCommand) {
            return JobDispatcher::submitAfter(fenceID, _nextType, std::forward<Command>(_nextCommand));
        }
    };

    // --- Template Implementations ---

    template<typename Command>
    sJobHandle JobDispatcher::submit(const eJobType _type, Command &&_command) {
        if (!initialized.load(std::memory_order_acquire)) return {0};

        const uint32_t fenceID = s_nextFenceID.fetch_add(1, std::memory_order_relaxed);
        s_fencePool[fenceID % MAX_FENCES].store(1, std::memory_order_release);

        sTask newTask;
        newTask.fenceID = fenceID;

        using CmdType = std::decay_t<Command>;
        newTask.execute = [cmd = CmdType(std::forward<Command>(_command))]() mutable { cmd(); };


        dispatchInternal(_type, std::move(newTask));
        return {fenceID};
    }

    template<typename Command>
    sJobHandle JobDispatcher::submitAfter(uint32_t _dependencyFence, const eJobType _type, Command &&_command) {
        const uint32_t myFence = s_nextFenceID.fetch_add(1, std::memory_order_relaxed);
        s_fencePool[myFence % MAX_FENCES].store(1, std::memory_order_release);

        sTask newTask;
        newTask.fenceID = myFence;
        using CmdType = std::decay_t<Command>;
        newTask.execute = [cmd = CmdType(std::forward<Command>(_command))]() mutable { cmd(); }; {
            std::lock_guard lock(s_dependencyMutex);
            if (isFenceSignaled(_dependencyFence)) {
                dispatchInternal(_type, std::move(newTask));
            } else {
                s_dependencies[_dependencyFence].push_back({_type, std::move(newTask)});
            }
        }
        return {myFence};
    }
}
