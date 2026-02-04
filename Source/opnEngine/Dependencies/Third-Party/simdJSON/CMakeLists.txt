add_library(simdjson)

target_sources(simdjson
        PRIVATE
        simdjson.cpp
)

target_sources(simdjson
        PUBLIC
        FILE_SET HEADERS
        FILES
        simdjson.cpp
)

target_include_directories(simdjson PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    target_compile_options(simdjson PRIVATE /W3)
else()
    target_compile_options(simdjson PRIVATE -Wall -Wextra)
endif()