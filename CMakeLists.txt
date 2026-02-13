cmake_minimum_required(VERSION 3.28.3)

set(CMAKE_CXX_SCAN_FOR_MODULES ON)

if (WIN32 AND CMAKE_C_COMPILER_ID MATCHES "Clang")
    find_program(CMAKE_RC_COMPILER NAMES rc)
    if (NOT CMAKE_RC_COMPILER)
        message(FATAL_ERROR "Could not find rc.exe. Please install Windows SDK or add rc.exe location to PATH")
    endif ()
endif ()

project(OPN_Engine
        VERSION 0.0.1
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(OPN_BUILD_APP "Build Application target" ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_subdirectory(Source/opnEngine)
if (OPN_BUILD_APP)
    add_subdirectory(App)
endif ()