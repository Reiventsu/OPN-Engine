add_library(EntityComponentSystem)

target_sources(EntityComponentSystem
        PUBLIC
        FILE_SET CXX_MODULES FILES
        Registry.cppm
        ECS.cppm
)

target_link_libraries(EntityComponentSystem PUBLIC components)