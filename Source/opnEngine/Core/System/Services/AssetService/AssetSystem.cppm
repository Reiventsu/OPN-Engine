module;

#include <functional>
#include <string>
#include <thread>
#include <vector>
#include <shared_mutex>

#include <fastgltf/core.hpp>

export module opn.System.Service.AssetSystem;
import opn.System.ServiceInterface;
import opn.System.JobDispatcher;
import opn.Utils.Logging;
import opn.System.UUID;
import opn.System.ServiceManager;

import opn.Assets.AssetDefines;

namespace opn {
    struct cmd_UploadMesh;
    struct cmd_UploadTexture;

    using uAsset = std::variant< Mesh, Texture, Material >;

    enum class eAssetErrorType {
        FileNotFound,
        ParsingError,
        UnsupportedFileType,
        OutOfMemory,
        InvalidData,
    };

    enum class eAssetState {
        Uninitialized,
        Loading,
        Uploading,
        Ready,
        Failed,
    };

    struct sAssetError {

        eAssetErrorType type;
        std::string path;
        std::string details;

        [[nodiscard]] std::string toString() const {
            return std::format("[{}] {}: {}", errorTypeToString(type), path, details);
        };

    private:
        static std::string_view errorTypeToString(const eAssetErrorType _type) {
            switch (_type) {
                case eAssetErrorType::FileNotFound:        return "FileNotFound";
                case eAssetErrorType::ParsingError:        return "ParsingError";
                case eAssetErrorType::UnsupportedFileType: return "UnsupportedFileType";
                case eAssetErrorType::OutOfMemory:         return "OutOfMemory";
                case eAssetErrorType::InvalidData:         return "InvalidData";
                default: return "Unknown";
            }
        }
    };

    struct sAssetMetadata {
        eAssetState state = eAssetState::Uninitialized;
        std::string path;
        std::variant< opn::Mesh, opn::Texture, opn::Material > data;
    };

    struct sAssetHandle {
        UUID id;
        constexpr sAssetHandle() : id{} {}
        constexpr sAssetHandle(UUID _id) : id{_id} {}

        bool isValid() const { return id.isValid(); };

        bool operator==(const sAssetHandle&) const = default;
    };

    struct sAssetHandleHasher {
        size_t operator()(const sAssetHandle& _handle) const {
            return UUIDHasher{}(_handle.id);
        }
    };

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

        std::unordered_map< UUID, uAsset, UUIDHasher > m_assets;
        std::unordered_map< std::string, UUID >        m_pathToHandle;
        std::unordered_map< UUID, size_t, UUIDHasher > m_refCounts;

        mutable std::shared_mutex m_assetsMutex;
        fastgltf::Parser m_fastgltfParser;
        std::vector<std::jthread> m_workers;

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


        /// Asset functions n stuff


        /// TODO make these not dummy functions

        void loadInternal(const std::string &_path) {}

        void unloadInternal(const std::string &_path) {}
    };

    void CommandAssetLoad::operator()() const {
        if (const auto *sys = AssetSystem::getService()) {
            sys->loadInternal(assetPath);
        } else {
            logError("AssetService", "Failed to load asset {} as AssetSystem is not initialized.", assetPath);
        }
    }

    void CommandAssetUnload::operator()() const {
        if (const auto *sys = AssetSystem::getService()) {
            sys->unloadInternal(assetPath);
        }
    }
}
