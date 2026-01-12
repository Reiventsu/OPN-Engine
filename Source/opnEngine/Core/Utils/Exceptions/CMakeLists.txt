add_library(Exceptions)

# Add shared utilities that never change
target_sources(Exceptions
        PUBLIC
        FILE_SET CXX_MODULES FILES
        Except_MultipleInit.cppm
)
