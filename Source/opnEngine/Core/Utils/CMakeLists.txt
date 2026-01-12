add_library(CoreUtils)

# Add shared utilities that never change
target_sources(CoreUtils
        PUBLIC
        FILE_SET CXX_MODULES FILES
        iSingleton.cppm
)

# Only add the Linux logger to the module set if we aren't on Windows
if(NOT WIN32)
    target_sources(CoreUtils
            PUBLIC
            FILE_SET CXX_MODULES FILES
            Logger.cppm
    )
endif()

add_subdirectory(Exceptions)
target_link_libraries(CoreUtils PUBLIC Exceptions)