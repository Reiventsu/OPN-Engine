#pragma once
#include <filesystem>

namespace opn {
    inline std::filesystem::path assetsRoot() {
#ifdef OPN_ASSETS_ROOT
        return std::filesystem::path(OPN_ASSETS_ROOT);
#else
        return std::filesystem::current_path() / ".." / ".." / ".." / "Assets";
#endif
    }
    inline std::filesystem::path shadersRoot()  { return assetsRoot() / "Shaders";  }
    inline std::filesystem::path meshesRoot()   { return assetsRoot() / "Meshes";   }
    inline std::filesystem::path texturesRoot() { return assetsRoot() / "Textures"; }
}