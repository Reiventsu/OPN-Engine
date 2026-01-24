add_subdirectory(vk-bootstrap)
add_subdirectory(HLSL++)
add_subdirectory(VulkanMemoryAllocator)

add_library(Third-Party INTERFACE)
target_link_libraries(Third-Party INTERFACE
        vk-bootstrap
        HLSL++
        VulkanMemoryAllocator
)