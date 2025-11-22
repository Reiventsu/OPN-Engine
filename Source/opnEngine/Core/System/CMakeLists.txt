add_library(CoreSystems INTERFACE
    JobDispatcher.h
    UUID.h
)

add_subdirectory(Asset)

target_include_directories(CoreSystems INTERFACE .)