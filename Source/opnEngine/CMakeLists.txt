add_subdirectory(Core/Utils)
add_subdirectory(Core/Renderer)
add_subdirectory(Core/Shaders)

add_executable(OPN_Engine
        Standalone/main.cpp)

target_link_libraries(OPN_Engine
    PRIVATE
        Renderer   # Your Renderer lib
        CoreUtils  # Your new RingBuffer lib
)

add_dependencies(OPN_Engine Shaders)