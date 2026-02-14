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
# Needed to do this to make linker see the symbols
target_link_libraries(OPN_App PRIVATE
    OPN::EngineMain
    -Wl,--whole-archive
    ApplicationCode
    -Wl,--no-whole-archive
)