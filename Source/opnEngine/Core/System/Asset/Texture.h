#pragma once
#include "iAsset.h"

namespace opn::AssetData {
    class Texture final : public iAsset {
    public:
        std::vector<uint8_t> pixel_data;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t channels = 4;

        Texture(const std::string_view _name, const std::string_view _path)
            : iAsset(eAssetType::Texture, _name, _path) {
        };

        void freeCPUData() override {
            pixel_data.clear();
            pixel_data.shrink_to_fit();
            isCPULoaded = false;
        }
    };
}
