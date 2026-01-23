find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
add_subdirectory(Plugins)
add_subdirectory(Core)

add_executable(OPN_Engine
        Standalone/main.cpp)

target_link_libraries(OPN_Engine
        PRIVATE
        GPUOpen::VulkanMemoryAllocator
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