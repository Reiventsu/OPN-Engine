find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

# Added in order of dependency
add_subdirectory(Dependencies)
add_subdirectory(Core)
add_subdirectory(Entrypoint)
