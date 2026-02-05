cmake_minimum_required(VERSION 4.1)

project(OPN_Application
        VERSION 0.0.1
        LANGUAGES CXX
)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../Source/)

add_library(ApplicationCode)

target_sources(ApplicationCode
PUBLIC
FILE_SET CXX_MODULES FILES
        App.cppm
)

add_executable(UserApp )

