module;

#include <functional>
#include <mutex>
#include <string>
#include <shared_mutex>

#include <fastgltf/core.hpp>

export module opn.System.Service.AssetSystem;
import opn.System.ServiceInterface;
import opn.System.Jobs.Types;
import opn.Utils.Locator;
import opn.Utils.Logging;
import opn.System.UUID;
import opn.Assets.Defines;
import opn.Assets.Types;

namespace opn {
    using tAsset = std::variant<Mesh, Texture, Material>;

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

    struct sAssetMetadata {
        eAssetState state = eAssetState::Uninitialized;
        std::string path;
        tAsset data;
    };

    export class AssetSystem;

    export struct CommandAssetLoad {
        std::string assetPath;
        void operator()();
    };

    export struct CommandAssetUnload {
        std::string assetPath;
        void operator()();
    };

    export class AssetSystem final : public Service<AssetSystem> {
        friend struct CommandAssetLoad;
        friend struct CommandAssetUnload;

        std::unordered_map<UUID, tAsset, UUIDHasher> m_assets;
        std::unordered_map<std::string, UUID> m_pathToHandle;
        std::unordered_map<UUID, size_t, UUIDHasher> m_refCounts;
        mutable std::shared_mutex m_assetsMutex;
        fastgltf::Parser m_gltfParser;

    public:
        [[nodiscard]] static CommandAssetLoad load(std::string path) {
            return CommandAssetLoad{std::move(path)};
        }

        [[nodiscard]] static CommandAssetUnload unload(std::string path) {
            return CommandAssetUnload{std::move(path)};
        }

        void onInit() override {
            opn::logInfo("AssetService", "Initialized.");
        }

        void onShutdown() override {
            std::unique_lock lock(m_assetsMutex);
            m_assets.clear();
            m_pathToHandle.clear();
            m_refCounts.clear();
            opn::logInfo("AssetService", "Shutdown successfully.");
        }

        void onUpdate(float _deltaTime) override {
        }

    private:
        void loadInternal(const std::string &_path) {
            Locator::submit(eJobType::Asset, [this, _path]() {
                // TODO: parse with m_gltfParser, populate m_assets
                opn::logInfo("AssetService", "Loading asset: {}", _path);
            });
        }

        void unloadInternal(const std::string &_path) {
            std::unique_lock lock(m_assetsMutex);
            if (auto itr = m_pathToHandle.find(_path); itr != m_pathToHandle.end()) {
                m_assets.erase(itr->second);
                m_refCounts.erase(itr->second);
                m_pathToHandle.erase(itr);
            }
        }
    };

    void CommandAssetLoad::operator()() {
        if (auto *sys = Locator::getService<AssetSystem>()) {
            sys->loadInternal(assetPath);
        } else {
            logError("AssetService", "Failed to load asset {} - AssetSystem not found.", assetPath);
        }
    }

    void CommandAssetUnload::operator()() {
        if (auto *sys = Locator::getService<AssetSystem>()) {
            sys->unloadInternal(assetPath);
        }
    }
}
