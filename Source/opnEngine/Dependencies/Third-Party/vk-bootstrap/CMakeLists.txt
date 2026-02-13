add_library(vk-bootstrap STATIC)

target_sources(vk-bootstrap
        PRIVATE
        VkBootstrap.cpp
        PUBLIC
        VkBootstrap.h
        VkBootstrapDispatch.h
        VkBootstrapFeatureChain.h
        VkBootstrapFeatureChain.inl
)

target_include_directories(vk-bootstrap PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(vk-bootstrap PUBLIC Vulkan::Headers)