module;
#include <expected>
#include <filesystem>
#include <variant>

#include "fastgltf/core.hpp"
#include "fastgltf/util.hpp"
#include "fastgltf/base64.hpp"

export module opn.System.Render.Util.vk.vkLoader;
import opn.Rendering.Util.vk.vkTypes;
import opn.Utils.Logging;

namespace opn {
    class VulkanImpl;
}

export namespace opn::vkUtil {
    struct sGeometrySurface {
        uint32_t startIndex;
        uint32_t count;
    };
    struct sMeshAsset {
        std::string name;
        std::vector<sGeometrySurface> surfaces;
        sGPUMeshBuffers meshBuffers;
    };


    std::expected<std> loadMeshesGLTF(VulkanImpl* _enigne, std::filesystem::path _path ) {
        opn::logInfo("VulkanBackend:", "Loading GLTF: {}", _path.string() );

        fastgltf::GltfDataBuffer data;
        data.FromPath(_path);

        constexpr auto gltfOptions = fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;

        fastgltf::Asset asset;
        fastgltf::Parser parser;

        auto load = parser.loadGltfBinary(data, _path.parent_path(), gltfOptions);
        if (load) {
            asset = std::move(load.get());
        } else {
            opn::logError("VulkanBackend", "Failed too load GLTF: {}", fastgltf::to_underlying(load.error()));
            return {};
        }

    }
}
