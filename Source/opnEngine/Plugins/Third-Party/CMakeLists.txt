add_subdirectory(vk-bootstrap)

add_library(Third-Party INTERFACE)
target_link_libraries(Third-Party INTERFACE vk-bootstrap)