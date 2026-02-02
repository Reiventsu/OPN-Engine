module;
#include <atomic>
#include <string>
#include <vector>

export module opn.Assets.meshDefine;

export namespace opn {
    struct Mesh {
        std::vector<float> positions;
        std::vector<float> normals;
        std::vector<float> uvs;
        std::string name;

        std::atomic_bool uploadedToGPU = false;
        uint32_t vboHandle = 0;
        uint32_t iboHandle = 0;
    };
}
