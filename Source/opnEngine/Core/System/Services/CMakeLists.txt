add_library(WindowingInterface)

target_sources(WindowingInterface
        PUBLIC
        FILE_SET CXX_MODULES FILES
        WindowSurfaceProvider.cppm
)

target_link_libraries(WindowingInterface
        PUBLIC
        glfw
)

add_library(Services)

target_sources(Services
        PUBLIC
        FILE_SET CXX_MODULES FILES
        Time.cppm
        WindowSystem.cppm
        Rendering.cppm
        AssetService/AssetSystem.cppm
        SystemsPackage.cppm
)

target_link_libraries(Services
        PUBLIC
        CoreSystems
        CoreUtils
        AssetTypes
        AssetDefines
        WindowingInterface
        RenderInterface
        EntityComponentSystem
        glfw
        RenderHelpers
        imgui
)
