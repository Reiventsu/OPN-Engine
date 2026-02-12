add_library(EngineServices)

target_sources(EngineServices
        PUBLIC
        FILE_SET CXX_MODULES FILES
        EngineServices.cppm
)

target_link_libraries(EngineServices
        PUBLIC
        Application
        CoreSystems
        Services
        Renderer
)

add_library(EngineMain OBJECT main.cpp)

add_library(OPN::EngineMain ALIAS EngineMain)