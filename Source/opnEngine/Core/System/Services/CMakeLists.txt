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
        ShaderReflection.cppm
        Rendering.cppm
        AssetService/AssetSystem.cppm
        SystemsPackage.cppm

        PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/../../Locator/Locator.cpp"
)

target_link_libraries(Services
        PUBLIC
        EngineLocator
        EngineServices
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
        slang::slang
)
