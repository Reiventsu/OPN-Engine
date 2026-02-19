add_library(AssetTypes)

target_sources(AssetTypes
        PUBLIC
        FILE_SET CXX_MODULES FILES
        AssetTypes.cppm

        PUBLIC
        AssetPaths.inl
)

target_link_libraries(AssetTypes
        PUBLIC
        CoreSystems
)

add_library(AssetDefines)

target_sources(AssetDefines
        PUBLIC
        FILE_SET CXX_MODULES FILES
        meshDefine.cppm
        textureDefine.cppm
        materialDefine.cppm
        AssetDefines.cppm
)

target_link_libraries(AssetDefines
        PUBLIC
        AssetTypes
        RenderHelpers
        CoreSystems
)