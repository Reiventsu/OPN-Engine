add_library(CoreUtils INTERFACE

        Thread/SPSCQueue.h
        Thread/RawSPSCQueue.h
        UUID.h
)

target_include_directories(CoreUtils INTERFACE .)