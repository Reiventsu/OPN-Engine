if(WIN32)
    add_subdirectory(Windows)
endif()

# You can add a 'Platform' meta-library here if you want to abstract it
add_library(Platform INTERFACE)
if(WIN32)
    target_link_libraries(Platform INTERFACE WindowsPlatform)
endif()