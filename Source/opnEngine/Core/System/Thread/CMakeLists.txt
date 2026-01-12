add_library(Thread)

target_sources(Thread
        PUBLIC
        FILE_SET CXX_MODULES FILES
        SPSCQueue.cppm
        RawSPSCQueue.cppm
)

target_link_libraries(Thread PUBLIC CoreUtils)