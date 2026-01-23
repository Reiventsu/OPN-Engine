add_library(CoreSystems)

target_sources(CoreSystems
        PUBLIC
        FILE_SET CXX_MODULES FILES
        JobDispatcher.cppm
        ServiceManager.cppm
        SystemTypeList.cppm
        iService.cppm
        UUID.cppm
)

target_link_libraries(CoreSystems
        PUBLIC
        CoreUtils
        Thread
)

add_subdirectory(Services)
add_subdirectory(Thread)
