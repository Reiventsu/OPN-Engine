add_library(CoreSystems)

target_sources(CoreSystems
    PUBLIC
    FILE_SET CXX_MODULES FILES
        JobDispatcher.cppm
        SystemsManager.cppm
        SystemTypeList.cppm
        Asset/AssetSystem.cppm
        UUID.cppm
        Thread/RawSPSCQueue.cppm
        Thread/SPSCQueue.cppm
)

target_link_libraries(CoreSystems PUBLIC CoreUtils)

target_include_directories(CoreSystems PUBLIC .)