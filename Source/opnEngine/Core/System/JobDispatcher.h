#pragma once

#include <functional>
#include <atomic>
#include <vector>
#include <thread>
#include <future>

#include "Thread/SPSCQueue.h"

namespace opn {
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
        static constexpr size_t MAX_FENCES = 4096;
        static constexpr size_t QUEUE_SIZE = 1024;

    public:
        static void initialize() {
            for (auto &f: s_fencePool) {
                f.store(0, std::memory_order_relaxed);
            }
        }

        template<typename Func, typename... Args>
        static uint32_t Submit(eJobType _type, Func &&_func, Args &&... args) {
            uint32_t fenceID = s_nextFenceID.fetch_add(1) % MAX_FENCES;
            s_fencePool[fenceID].store(1, std::memory_order_release);

            sTask newTask;
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
