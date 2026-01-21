 find_package(Vulkan REQUIRED)
 find_package(glfw3 REQUIRED)

 add_library(Renderer)

 target_link_libraries(Renderer
         PUBLIC
         Vulkan::Vulkan
         glfw
 )

 target_sources(Renderer
         PUBLIC
         FILE_SET CXX_MODULES FILES
         VulkanImpl.cppm
 )

 target_include_directories(Renderer PUBLIC .)