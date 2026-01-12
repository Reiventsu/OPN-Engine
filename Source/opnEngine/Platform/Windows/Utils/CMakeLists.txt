add_library(WinUtils)

target_sources(WinUtils
        PUBLIC
        FILE_SET CXX_MODULES FILES
        winLogger.cppm
)

target_link_libraries(WinUtils PUBLIC CoreUtils)