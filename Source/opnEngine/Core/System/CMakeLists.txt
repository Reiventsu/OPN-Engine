add_library(CoreSystems)

target_sources(CoreSystems
        PUBLIC
        FILE_SET CXX_MODULES FILES
        UUID.cppm
        SystemTypeList.cppm
        iService.cppm
        ServiceManager.cppm
        Jobs/JobTypes.cppm
        Jobs/JobHandle.cppm
        Jobs/JobDispatcher.cppm
)

target_link_libraries(CoreSystems
        PUBLIC
        CoreUtils
        Thread
)