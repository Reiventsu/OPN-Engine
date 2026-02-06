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

add_executable(OPN_Engine main.cpp)

target_link_libraries(OPN_Engine PUBLIC EngineServices)
add_dependencies(OPN_Engine CompileShaders)