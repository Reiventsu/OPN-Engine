add_library(CoreUtils)

target_sources(CoreUtils
        PUBLIC
        FILE_SET CXX_MODULES FILES
        iSingleton.cppm
        Logger.cppm
)


add_subdirectory(Exceptions)
target_link_libraries(CoreUtils PUBLIC Exceptions)