add_library(CoreUtils
        Logger.cppm)

target_sources(CoreUtils
    PUBLIC
    FILE_SET CXX_MODULES FILES
        Thread/SPSCQueue.cppm
        Thread/RawSPSCQueue.cppm
)

target_include_directories(CoreUtils PUBLIC .)