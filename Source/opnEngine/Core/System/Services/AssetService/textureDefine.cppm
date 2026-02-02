module;
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

export module opn.Assets.textureDefine;

export namespace opn {
    struct Texture {
        std::vector<std::byte> data;
        uint32_t width;
        uint32_t height;
        uint32_t channels;
        std::string name;

        std::atomic_bool uploadedToGpu = false;
        uint32_t gpuHandle = 0;
    };
}
