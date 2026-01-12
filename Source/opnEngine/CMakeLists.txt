add_subdirectory(Core)
add_subdirectory(Platform)

add_library(EngineUtils INTERFACE)
target_link_libraries(EngineUtils INTERFACE CoreUtils)

if(WIN32)
    target_link_libraries(EngineUtils INTERFACE WinUtils)
endif()

target_link_libraries(CoreUtils PUBLIC EngineUtils)

add_executable(OPN_Engine
        Standalone/main.cpp)

target_link_libraries(OPN_Engine
        PRIVATE
        EngineUtils
        CoreSystems
        Services
)

target_include_directories(OPN_Engine
        PUBLIC
        Core
        Plugins
)