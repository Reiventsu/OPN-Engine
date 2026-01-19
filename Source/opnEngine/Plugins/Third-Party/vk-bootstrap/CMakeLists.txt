add_library(vk-bootstrap)

target_sources(vk-bootstrap
    PRIVATE
    VkBootstrap.cpp
    VkBootstrap.h
    VkBootstrapDispatch.h
    VkBootstrapFeatureChain.h
    VkBootstrapFeatureChain.inl
)
target_include_directories(vk-bootstrap PUBLIC .)

target_link_libraries(vk-bootstrap PRIVATE Vulkan::Vulkan)
