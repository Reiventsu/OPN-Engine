add_subdirectory(Utils)

# If you want a single target to link against for all Windows-specific stuff:
add_library(WinPlatform INTERFACE)
target_link_libraries(WinPlatform INTERFACE WinUtils)