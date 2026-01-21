module;

#include <functional>
#include <thread>
#include <vector>

export module opn.System.Service.AssetSystem;
import opn.System.ServiceInterface;
import opn.System.JobDispatcher;
import opn.Utils.Logging;
import opn.System.ServiceManager;

namespace opn {
    export class AssetSystem;

    export struct CommandAssetLoad {
        std::string assetPath;

        void operator()() const;
    };

    export struct CommandAssetUnload {
        std::string assetPath;

        void operator()() const;
    };

    export class AssetSystem final : public Service<AssetSystem> {
        friend struct CommandAssetLoad;
        friend struct CommandAssetUnload;

    public:
        [[nodiscard]] static CommandAssetLoad load(std::string path) { return CommandAssetLoad{std::move(path)}; }
        [[nodiscard]] static CommandAssetUnload unload(std::string path) { return CommandAssetUnload{std::move(path)}; }

        void onInit() override {
            const auto threadCount = std::max(1u, std::thread::hardware_concurrency() / 4);

            m_workers.reserve(threadCount);
            for (size_t i = 0; i < threadCount; ++i) {
                m_workers.emplace_back([this](const std::stop_token &_st) {
                    workerLoop(_st);
                });
            }
        }

        void onShutdown() override {
            for (auto &worker: m_workers) {
                worker.request_stop();
            }
            JobDispatcher::wakeWorkers(eJobType::Asset);

            m_workers.clear();
            opn::logInfo("AssetService", "Shutdown successfully.");
        }

        void onUpdate(float _deltaTime) override {
        };

    private:
        void workerLoop(const std::stop_token &_stopToken) {
            auto &assetQueue = JobDispatcher::getQueue(eJobType::Asset);
            uint32_t lastSignal = 0;

            while (!_stopToken.stop_requested()) {
                sTask task;
                if (assetQueue >> task) {
                    if (task.execute) {
                        task.execute();

                        // Signal done
                        JobDispatcher::signalCompletion(task.fenceID);
                    }
                } else {
                    JobDispatcher::waitForWork(eJobType::Asset, lastSignal);
                }
            }
        }

        std::vector<std::jthread> m_workers;

        /// Asset functions n stuff


        /// TODO make these not dummy functions

        void loadInternal(const std::string &_path) const {
            logInfo("AssetSystem", "Worker loading: {}", _path);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            logInfo("AssetSystem", "Worker finished: {}", _path);
        }

        void unloadInternal(const std::string &_path) const {
            logInfo("AssetSystem", "Unloading: {}", _path);
        }
    };

    void CommandAssetLoad::operator()() const {
        if (const auto *sys = AssetSystem::getInstance()) {
            sys->loadInternal(assetPath);
        } else {
            logError("AssetService", "Failed to load asset {} as AssetSystem is not initialized.", assetPath);
        }
    }

    void CommandAssetUnload::operator()() const {
        if (const auto *sys = AssetSystem::getInstance()) {
            sys->unloadInternal(assetPath);
        }
    }
}
