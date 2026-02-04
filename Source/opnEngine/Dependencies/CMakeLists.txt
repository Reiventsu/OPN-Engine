add_subdirectory(Third-Party)

add_library(Plugins INTERFACE)

target_link_libraries(Plugins INTERFACE
        Third-Party
)
