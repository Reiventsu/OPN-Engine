add_library(Components)

target_sources(Components
        PUBLIC
        FILE_SET CXX_MODULES FILES
        Components.cppm
        Renderable.cppm
        Transform.cppm
        ShaderOverride.cppm
)

target_link_libraries(Components
        PUBLIC
        AssetTypes
        hlslpp
)