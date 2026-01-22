find_package(glfw3 REQUIRED)

add_library(Renderer)

target_link_libraries(Renderer
        PUBLIC
        Vulkan::Vulkan
        glfw
        vk-bootstrap
        hlslpp_module
        CoreUtils
)

target_sources(Renderer
        PUBLIC
        FILE_SET CXX_MODULES FILES
        VulkanImpl.cppm
        DirectXImpl.cppm
        RenderBackend.cppm
)