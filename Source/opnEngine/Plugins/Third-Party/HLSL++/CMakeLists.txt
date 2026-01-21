add_library(hlslpp_module)

target_sources(hlslpp_module
        PUBLIC
        FILE_SET CXX_MODULES FILES
        hlslpp.cppm
        PRIVATE
        hlsl++.natvis
)

target_include_directories(hlslpp_module
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/hlsl++)

target_compile_features(hlslpp_module PUBLIC cxx_std_23)