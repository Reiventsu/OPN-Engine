cmake_minimum_required( VERSION 4.0 )

project( myownengine )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )

set( CMAKE_CXX_STANDARD 23 )

add_subdirectory( Core/Renderer )

add_executable( myownengine
    Core/main.cpp
)

target_link_libraries( myownengine
    PRIVATE
        Renderer
)
