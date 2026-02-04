add_subdirectory(vk-bootstrap)
add_subdirectory(HLSL++)
add_subdirectory(VulkanMemoryAllocator)
add_subdirectory(simdJSON)
add_subdirectory(FastGLTF)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ImGui)

add_library(imgui INTERFACE)

target_compile_definitions(imgui INTERFACE
        IMGUI_USER_CONFIG="${CMAKE_SOURCE_DIR}/Source/opnEngine/Dependencies/Third-Party/OPN_imconfig.h"
)


target_include_directories(imgui INTERFACE
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
)
target_sources(imgui INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/OPN_imconfig.h
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
)

add_library(Third-Party INTERFACE)
target_link_libraries(Third-Party INTERFACE
        vk-bootstrap
        HLSL++
        VulkanMemoryAllocator
        imgui
        simdjson
        fastGLTF
)
