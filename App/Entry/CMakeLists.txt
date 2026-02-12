add_library(ApplicationCode)

target_sources(ApplicationCode
        PUBLIC
        FILE_SET CXX_MODULES FILES
        App.cppm
        PRIVATE
        App.cpp
)
target_link_libraries(ApplicationCode
        PUBLIC
        OPN::Engine
)

add_executable(OPN_App App.cpp)
target_link_libraries(OPN_App PRIVATE ApplicationCode OPN::EngineMain)