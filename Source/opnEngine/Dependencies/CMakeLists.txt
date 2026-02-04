add_subdirectory(Third-Party)

add_library(Dependencies INTERFACE)

target_link_libraries(Dependencies INTERFACE
        Third-Party
)
