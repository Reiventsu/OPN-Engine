cmake_minimum_required(VERSION 4.1)

project(OPN_Application
        VERSION 0.0.1
        LANGUAGES CXX
)

add_subdirectory(../../Source)

add_library(ApplicationCode)

target_sources(ApplicationCode
        PUBLIC
        FILE_SET CXX_MODULES FILES
        App.cppm
        App.cpp
)

target_link_libraries(ApplicationCode
        PUBLIC
        OPN_Engine
        CoreUtils
)

add_dependencies(ApplicationCode OPN_Engine)