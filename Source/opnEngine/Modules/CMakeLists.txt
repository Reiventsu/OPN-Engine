add_library(EngineModules)

target_sources(EngineModules
        PUBLIC
        FILE_SET CXX_MODULES FILES
        Module_ServicePackage.cppm

)

target_link_libraries(EngineModules
        PUBLIC
        Services
        EntityComponentSystem
        Renderer
)