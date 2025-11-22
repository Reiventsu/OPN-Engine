add_subdirectory(Core)

add_executable(OPN_Engine
        Standalone/main.cpp)

target_link_libraries(OPN_Engine
    PRIVATE
        Renderer
        CoreUtils
)

target_include_directories(OPN_Engine
    PUBLIC
        Core
        Modules
)

add_dependencies(OPN_Engine Shaders)