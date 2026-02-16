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
        Vulkan/vkTypes.cppm
        Vulkan/vkInit.cppm
        Vulkan/vkImage.cppm
        Vulkan/vkDescriptors.cppm
        Vulkan/vkPipeline.cppm
        Vulkan/vkUtil.cppm

        RendererTypes.cppm
)

target_link_libraries(RenderHelpers
        PUBLIC
        CoreUtils
        Vulkan::Headers
        VulkanMemoryAllocator
        glfw
        vk-bootstrap
        hlslpp
        simdjson
        fastgltf::fastgltf
)

add_library(Renderer)

target_sources(Renderer PRIVATE Vulkan/VMA_VOLK_IMPLEMENTATION.cpp)

target_sources(Renderer
        PUBLIC
        FILE_SET CXX_MODULES FILES
        RenderPackage.cppm
        Vulkan/VulkanImpl.cppm
        DirectX/DirectXImpl.cppm
)

target_link_libraries(Renderer
        PUBLIC
        RenderHelpers
        RenderInterface
        WindowingInterface
        imgui
        CoreSystems
)

add_dependencies(Renderer CompileShaders)
