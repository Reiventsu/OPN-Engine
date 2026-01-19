add_subdirectory(Core)

add_executable(OPN_Engine
        Standalone/main.cpp)

find_package(Vulkan REQUIRED)

target_link_libraries(OPN_Engine
        PRIVATE
        CoreUtils
        CoreSystems
        Services
)

target_include_directories(OPN_Engine
        PUBLIC
        Core
        Plugins
)