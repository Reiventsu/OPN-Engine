add_library(RenderInterface)

target_sources(RenderInterface
        PUBLIC
        FILE_SET CXX_MODULES FILES
        RenderBackend.cppm
)

target_link_libraries(RenderInterface PUBLIC WindowingInterface)

add_library(Renderer)

target_link_libraries(Renderer
        PUBLIC
        RenderInterface
        WindowingInterface
        CoreUtils
        Vulkan::Vulkan
        glfw
        vk-bootstrap
        hlslpp_module
)

target_sources(Renderer
        PUBLIC
        FILE_SET CXX_MODULES FILES
        Vulkan/VulkanImpl.cppm
        Vulkan/vkInit.cppm
        Vulkan/vkTypes.cppm
        DirectXImpl.cppm
        RenderPackage.cppm
)