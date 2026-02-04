add_library(components)

target_sources(components
        PUBLIC
        FILE_SET CXX_MODULES FILES
        Components.cppm
        Renderable.cppm
        Transform.cppm
)

target_link_libraries(components
        PUBLIC
        hlslpp
)