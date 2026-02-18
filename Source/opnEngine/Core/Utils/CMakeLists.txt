add_library(CoreUtils)

target_sources(CoreUtils
        PUBLIC
        FILE_SET CXX_MODULES FILES
        # Utilities
        Logger.cppm

        # Exceptions
        Exceptions/Except_MultipleInit.cppm

)
