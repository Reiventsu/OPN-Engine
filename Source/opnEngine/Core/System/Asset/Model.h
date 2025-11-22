#pragma once
#include "iAsset.h"

namespace opn::AssetData
{
    class Model final : public iAsset
    {
    public:
        Model(std::string_view _name, std::string_view _path)
    };
}
