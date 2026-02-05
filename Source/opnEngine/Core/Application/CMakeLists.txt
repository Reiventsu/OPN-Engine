add_library(Application)

target_sources(Application
        PUBLIC
        FILE_SET CXX_MODULES FILES
        Application.cppm
        ApplicationRunner.cppm
)

target_link_libraries(Application
        PUBLIC
        CoreUtils
        CoreSystems
        Services
)