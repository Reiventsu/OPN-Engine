add_subdirectory(EntityComponentSystem)

add_library(WindowingInterface)

target_sources(WindowingInterface
        PUBLIC
        FILE_SET CXX_MODULES FILES
        WindowSurfaceProvider.cppm
)

target_link_libraries(WindowingInterface
        PUBLIC
        Vulkan::Vulkan
)

add_library(Services)

target_sources(Services
        PUBLIC
        FILE_SET CXX_MODULES FILES
        # Asset related
        AssetService/meshDefine.cppm
        AssetService/textureDefine.cppm
        AssetService/materialDefine.cppm
        AssetService/AssetSystem.cppm
        AssetService/AssetDefines.cppm
        AssetService/AssetTypes.cppm

        # Others
        SystemsPackage.cppm
        Time.cppm
        WindowSystem.cppm
        Rendering.cppm
)

target_link_libraries(Services
        PUBLIC
        CoreSystems
        CoreUtils
        EntityComponentSystem
        WindowingInterface
        RenderInterface
        glfw
        RenderHelpers
        imgui
)
