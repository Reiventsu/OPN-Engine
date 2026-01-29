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
        SystemsPackage.cppm
        Time.cppm
        AssetSystem.cppm
        WindowSystem.cppm
        Rendering.cppm
)

target_link_libraries(Services
        PUBLIC
        CoreSystems
        CoreUtils
        WindowingInterface
        RenderInterface
        glfw
        RenderHelpers
        imgui
)
