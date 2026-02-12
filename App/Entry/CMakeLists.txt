add_library(ApplicationCode)

target_sources(ApplicationCode
        PUBLIC
        FILE_SET CXX_MODULES FILES
        App.cppm
)

target_sources(ApplicationCode
        PRIVATE
        App.cpp
)

target_link_libraries(ApplicationCode
        PUBLIC
        OPN_Engine
)

add_executable(OPN_App App.cpp)
target_link_libraries(OPN_App PRIVATE ApplicationCode)