add_library(RenderInterface)

target_sources(RenderInterface
        PUBLIC
        FILE_SET CXX_MODULES FILES
        RenderBackend.cppm
)

target_link_libraries(RenderInterface PUBLIC WindowingInterface)

add_library(RenderHelpers)

target_sources(RenderHelpers
        PUBLIC
        FILE_SET CXX_MODULES FILES
        Vulkan/vkUtil.cppm
        Vulkan/vkInit.cppm
        Vulkan/vkTypes.cppm
        Vulkan/vkImage.cppm
        Vulkan/vkDescriptors.cppm
        Vulkan/vkPipeline.cppm
)

target_link_libraries(RenderHelpers
        PUBLIC
        CoreUtils
        Vulkan::Vulkan
        VulkanMemoryAllocator
        glfw
        vk-bootstrap
        hlslpp
)

add_library(Renderer)

target_link_libraries(Renderer
        PUBLIC
        RenderHelpers
        RenderInterface
        WindowingInterface
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

        DirectX/DirectXImpl.cppm
)
