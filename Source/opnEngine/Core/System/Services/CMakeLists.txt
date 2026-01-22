add_library(WindowingInterface)

target_sources(WindowingInterface
        PUBLIC
        FILE_SET CXX_MODULES FILES
        WindowSurfaceProvider.cppm
)

add_library(Services)

target_sources(Services
        PUBLIC
        FILE_SET CXX_MODULES FILES
        SystemsPackage.cppm
        AssetSystem.cppm
        WindowSystem.cppm
        Time.cppm
        Rendering.cppm
)

target_link_libraries(Services
        PUBLIC
        CoreSystems
        CoreUtils
        WindowingInterface
        RenderInterface
        glfw
)