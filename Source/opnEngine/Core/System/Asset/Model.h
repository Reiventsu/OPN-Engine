#pragma once
#include "iAsset.h"
#include "Vertex.h"

namespace opn::AssetData {
    class Model final : public iAsset {
    public:
        std::vector<sVertex> vertices;
        std::vector<uint32_t> indices;

        Model(const std::string_view _name, const std::string_view _path)
            : iAsset(eAssetType::Model, _name, _path) {
        };

        void freeCPUData() override {
            vertices.clear();
            vertices.shrink_to_fit();
            indices.clear();
            indices.shrink_to_fit();
            isCPULoaded = false;
        }
    };
}
