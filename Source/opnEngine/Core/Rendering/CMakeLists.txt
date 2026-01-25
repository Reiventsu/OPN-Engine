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
        VulkanMemoryAllocator
        glfw
        vk-bootstrap
        hlslpp
)

target_sources(Renderer
        PUBLIC
        Vulkan/VolkImplementation.cpp
        Vulkan/VMAImplementation.cpp
)

target_sources(Renderer
        PUBLIC
        FILE_SET CXX_MODULES FILES

        RenderPackage.cppm

        Vulkan/VulkanImpl.cppm
        Vulkan/vkInit.cppm
        Vulkan/vkTypes.cppm
        Vulkan/vkImage.cppm
        Vulkan/vkDescriptors.cppm
        Vulkan/vkPipeline.cppm

        DirectX/DirectXImpl.cppm
)