find_package(Vulkan REQUIRED)
add_subdirectory(Plugins)
add_subdirectory(Core)

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
)

target_include_directories(OPN_Engine
        PUBLIC
        Plugins
        Core
)