find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

add_subdirectory(Plugins)
add_subdirectory(Core)
add_subdirectory(Core/Shaders)

add_library(EngineServices)

target_sources(EngineServices
        PUBLIC
        FILE_SET CXX_MODULES FILES
        Standalone/EngineServices.cppm
)

target_link_libraries(EngineServices
        PUBLIC
        CoreSystems
        Services
        Renderer
)

add_executable(OPN_Engine
        Standalone/main.cpp)

target_link_libraries(OPN_Engine
        PRIVATE
        CoreUtils
        CoreSystems
        WindowingInterface
        RenderInterface
        Services
        Renderer
        EngineServices
)

target_include_directories(OPN_Engine
        PUBLIC
        Plugins
        Core
)

add_dependencies(OPN_Engine CompileShaders)
