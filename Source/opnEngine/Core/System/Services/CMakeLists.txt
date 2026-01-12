add_library(Services)

target_sources(Services
        PUBLIC
        FILE_SET CXX_MODULES FILES
        SystemsPackage.cppm
        AssetSystem.cppm
        TestSystem.cppm
)

target_link_libraries(Services PUBLIC CoreSystems CoreUtils)