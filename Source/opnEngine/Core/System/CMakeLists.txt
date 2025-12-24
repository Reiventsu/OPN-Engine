add_library(CoreSystems)

target_sources(CoreSystems
    PUBLIC
    FILE_SET CXX_MODULES FILES
        JobDispatcher.cppm
        UUID.cppm
)

target_link_libraries(CoreSystems PUBLIC CoreUtils)

add_subdirectory(Asset)

target_include_directories(CoreSystems PUBLIC .)