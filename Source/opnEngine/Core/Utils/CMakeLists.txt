add_library(CoreUtils INTERFACE

        Thread/SPSCQueue.h
        Thread/RawSPSCQueue.h
)

target_include_directories(CoreUtils INTERFACE .)