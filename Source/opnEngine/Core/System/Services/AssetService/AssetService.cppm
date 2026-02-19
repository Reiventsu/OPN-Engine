module;

#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <shared_mutex>

#include <fastgltf/core.hpp>
#include "AssetPaths.inl"

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

    enum class eAssetType {
        Mesh,
        Texture,
        Shader,
        Unknown,
    };

    struct sAssetDescriptor {
        std::filesystem::path canonicalPath;
        eAssetType            type;
        eAssetState           state = eAssetState::Uninitialized;
    };

    struct sAssetMetadata {
        tAsset data;
    };

    export class AssetService;

    export struct CommandAssetLoad {
        std::string assetPath;
        void operator()();
    };

    export struct CommandAssetUnload {
        std::string assetPath;
        void operator()();
    };

    export class AssetService final : public Service<AssetService> {
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
            opn::logInfo("AssetService", "Initializing...");
            scanDirectory(assetsRoot());
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
        std::unordered_map<std::string, sAssetDescriptor> m_assetRegistry;

        static eAssetType classifyExtension(const std::filesystem::path& _ext) {
            if (_ext == ".slang")                      return eAssetType::Shader;
            if (_ext == ".gltf" || _ext == ".glb")     return eAssetType::Mesh;
            if (_ext == ".png"  || _ext == ".jpg"
             || _ext == ".jpeg" || _ext == ".ktx")     return eAssetType::Texture;
            return eAssetType::Unknown;
        }

        void scanDirectory(const std::filesystem::path& _root) {
            if (!std::filesystem::exists(_root)) {
                opn::logWarning("AssetService", "Asset directory not found: {}", _root.string());
                return;
            }

            for (const auto& entry : std::filesystem::recursive_directory_iterator(_root)) {
                if (!entry.is_regular_file()) continue;

                const auto ext = entry.path().extension();
                const auto type = classifyExtension(ext);

                if (type == eAssetType::Unknown) {
                    opn::logInfo("AssetService", "Ignoring unrecognised file: {}", entry.path().filename().string());
                    continue;
                }

                auto canonical = std::filesystem::canonical(entry.path());
                auto root = std::filesystem::canonical(_root);

                if (canonical.string().find(root.string()) != 0) {
                    opn::logWarning("AssetService", "Rejecting path outside assets root: {}", canonical.string());
                    continue;
                }

                const auto key = canonical.string();
                m_assetRegistry[key] = sAssetDescriptor{
                    .canonicalPath = canonical,
                    .type = type,
                    .state = eAssetState::Uninitialized
                };

                opn::logDebug("AssetService", "Discovered [{}]: {}", ext.string(), canonical.filename().string() );
            }
            opn::logInfo("AssetService", "Asset scan complete. {} assets discovered.", m_assetRegistry.size());
        }

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
        if (auto *sys = Locator::getService<AssetService>()) {
            sys->loadInternal(assetPath);
        } else {
            logError("AssetService", "Failed to load asset {} - AssetSystem not found.", assetPath);
        }
    }

    void CommandAssetUnload::operator()() {
        if (auto *sys = Locator::getService<AssetService>()) {
            sys->unloadInternal(assetPath);
        }
    }
}
