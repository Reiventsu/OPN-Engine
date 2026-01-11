add_library(CoreSystems)

target_sources(CoreSystems
    PUBLIC
    FILE_SET CXX_MODULES FILES
        JobDispatcher.cppm
        ServiceManager.cppm
        SystemTypeList.cppm
        iService.cppm
        UUID.cppm
        Thread/RawSPSCQueue.cppm
        Thread/SPSCQueue.cppm
)

add_subdirectory(Services)

target_link_libraries(CoreSystems PUBLIC CoreUtils)

target_include_directories(CoreSystems PUBLIC .)