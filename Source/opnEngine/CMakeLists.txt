find_package(VulkanHeaders CONFIG REQUIRED)
find_package(volk CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

# Added in order of dependency
add_subdirectory(Dependencies)
add_subdirectory(Core)
add_subdirectory(Entrypoint)

add_library(OPN_Engine INTERFACE)
add_library(OPN::Engine ALIAS OPN_Engine)

target_link_libraries(OPN_Engine
        INTERFACE
        Application
        EngineServices
)