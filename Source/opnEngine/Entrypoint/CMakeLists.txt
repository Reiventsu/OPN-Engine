add_library(EngineMain OBJECT main.cpp)

target_link_libraries(EngineMain
        PRIVATE
        Application
)

add_library(OPN::EngineMain ALIAS EngineMain)