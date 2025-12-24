add_library(CoreUtils)

target_sources(CoreUtils
    PUBLIC
    FILE_SET CXX_MODULES FILES
        Logger.cppm
        Exceptions/Except_MultipleInit.cppm
)

target_include_directories(CoreUtils PUBLIC .)