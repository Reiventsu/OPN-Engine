add_library(EntityComponentSystem)

target_sources(EntityComponentSystem
        PUBLIC
        FILE_SET CXX_MODULES FILES
        Registry.cppm
        Service.cppm
        Systems.cppm
        tEntity.cppm
        ECB.cppm
        ECS.cppm
)

target_link_libraries(EntityComponentSystem
        PUBLIC
        Components
        CoreSystems
)