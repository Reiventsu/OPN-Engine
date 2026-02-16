add_library(Application)

target_sources(Application
        PUBLIC
        FILE_SET CXX_MODULES FILES
        iApplication.cppm
        AppLinker.cppm
        Application.cppm
        EntryPoint.cppm
)

target_link_libraries(Application
        PUBLIC
        EngineServices
        CoreUtils
        EngineLocator
)