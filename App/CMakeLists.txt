add_library(ApplicationCode)

target_sources(ApplicationCode
        PUBLIC
        FILE_SET CXX_MODULES FILES
        Entry/App.cppm
        PRIVATE
        Entry/App.cpp
)
target_link_libraries(ApplicationCode
        PUBLIC
        OPN::Engine
)

add_executable(OPN_App)
target_link_libraries(OPN_App PRIVATE ApplicationCode OPN::EngineMain)
