add_library(SystemAssets INTERFACE
    iAsset.h
    Vertex.h
    Model.h
        Texture.h
        AssetSystem.h
)

target_include_directories(SystemAssets INTERFACE .)