#pragma once
#include <atomic>
#include <string>

#include "Utils/UUID.h"

namespace opn::AssetData
{
    enum class eAssetType
    {
        Texture,
        Model,
        Material,
        Shader,
        Animation,
        Audio,
        Font,
        Scene,
        Prefab,
    };

    class iAsset
    {
    public:
        virtual ~iAsset() = default;

        UUID        uuid;
        eAssetType  type;
        std::string name;
        std::string path;

        std::atomic<bool> isCPULoaded = false;
        std::atomic<bool> isGPULoaded = false;

    protected:
        iAsset(const eAssetType _type, const std::string_view _name, const std::string_view _path)
            : type(_type), name(_name), path(_path)
        {
        }
    };
}
