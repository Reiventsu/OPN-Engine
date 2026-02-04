add_library(hlslpp INTERFACE)
target_compile_definitions(hlslpp INTERFACE HLSLPP_FEATURE_TRANSFORM)

target_sources(hlslpp
        INTERFACE
        FILE_SET HEADERS
        FILES
        hlsl++.h
)

target_include_directories(hlslpp
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/hlsl++
)