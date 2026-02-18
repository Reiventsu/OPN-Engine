add_library(EngineLocator)

target_sources(EngineLocator
        PUBLIC
        FILE_SET CXX_MODULES FILES
        Locator.cppm
)

target_link_libraries(EngineLocator
        PUBLIC
        CoreUtils
        CoreSystems
)