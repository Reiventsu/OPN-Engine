module;
#include <unordered_map>
#include <filesystem>
export module opn.System.Render.Util.vk.vkLoader;
import opn.Rendering.Util.vk.vkTypes;

export namespace opn::vkUtil {
    struct sGeometrySurface {
        uint32_t startIndex;
        uint32_t count;
    };
    struct sMeshAsset {
        std::string name;
        std::vector<sGeometrySurface> surfaces;
        vkUtil::sGPUMeshBuffers
    };
}