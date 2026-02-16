add_library(EngineServices)

target_sources(EngineServices
        PUBLIC
        FILE_SET CXX_MODULES FILES
        EngineServices.cppm
)

target_link_libraries(EngineServices
        PUBLIC
        CoreSystems
        EngineLocator
        Services
        Renderer
)

add_library(EngineMain OBJECT main.cpp)

target_link_libraries(EngineMain
        PRIVATE
        Application
)

add_library(OPN::EngineMain ALIAS EngineMain)
