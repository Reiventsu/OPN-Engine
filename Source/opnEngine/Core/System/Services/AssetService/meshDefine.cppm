module;
#include <atomic>
#include <string>
#include <vector>

export module opn.Assets.meshDefine;
import opn.Rendering.Util.vk.vkTypes;

export namespace opn {
    struct Mesh {
        std::vector<float> positions;
        std::vector<float> normals;
        std::vector<float> uvs;
        std::vector<uint32_t> indices;
        std::string name;

        std::atomic_bool uploadedToGPU = false;
        vkUtil::sGPUMeshBuffers gpuMeshBuffers;
        uint32_t indexCount;
    };
}
