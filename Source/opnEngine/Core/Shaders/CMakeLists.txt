# Find all GLSL shader files
file(GLOB_RECURSE GLSL_SOURCE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/GLSL/*.frag"
        "${CMAKE_CURRENT_SOURCE_DIR}/GLSL/*.vert"
)

# Find all HLSL shader files
file(GLOB_RECURSE HLSL_SOURCE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/HLSL/*.hlsl"
)

message(STATUS "Found GLSL shader files: ${GLSL_SOURCE_FILES}")
message(STATUS "Found HLSL shader files: ${HLSL_SOURCE_FILES}")

# Find glslc for GLSL compilation
find_program(GLSLC glslc)
if(NOT GLSLC)
    message(FATAL_ERROR "glslc not found! Please install Vulkan SDK")
endif()

# Find dxc for HLSL compilation
find_program(DXC dxc)
if(NOT DXC AND HLSL_SOURCE_FILES)
    message(WARNING "dxc not found! HLSL shaders will not be compiled. Install DirectXShaderCompiler.")
endif()

message(STATUS "glslc found at: ${GLSLC}")
if(DXC)
    message(STATUS "dxc found at: ${DXC}")
endif()

# Put compiled shaders next to the executable in bin/Shaders
set(SHADER_OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Shaders")
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

# Lists of compiled outputs
set(GLSL_SPIRV_FILES "")
set(HLSL_SPIRV_FILES "")

# Compile GLSL shaders with glslc
foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${SHADER_OUTPUT_DIR}/${FILE_NAME}.spv")

    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${GLSLC} ${GLSL} -o ${SPIRV}
            DEPENDS ${GLSL}
            COMMENT "Compiling GLSL shader: ${FILE_NAME}"
    )

    list(APPEND GLSL_SPIRV_FILES ${SPIRV})
endforeach()

# Compile HLSL shaders with dxc
if(DXC)
    foreach(HLSL ${HLSL_SOURCE_FILES})
        get_filename_component(FILE_NAME ${HLSL} NAME)
        set(SPIRV "${SHADER_OUTPUT_DIR}/${FILE_NAME}.spv")

        # Determine shader stage and target profile
        if(FILE_NAME MATCHES ".*\\.vert\\.hlsl$")
            set(SHADER_PROFILE "vs_6_0")
        elseif(FILE_NAME MATCHES ".*\\.frag\\.hlsl$")
            set(SHADER_PROFILE "ps_6_0")
        elseif(FILE_NAME MATCHES ".*\\.comp\\.hlsl$")
            set(SHADER_PROFILE "cs_6_0")
        elseif(FILE_NAME MATCHES ".*\\.geom\\.hlsl$")
            set(SHADER_PROFILE "gs_6_0")
        else()
            message(WARNING "Could not determine shader stage for ${FILE_NAME}, skipping")
            continue()
        endif()

        add_custom_command(
                OUTPUT ${SPIRV}
                COMMAND ${DXC} -spirv -T ${SHADER_PROFILE} -E main ${HLSL} -Fo ${SPIRV}
                DEPENDS ${HLSL}
                COMMENT "Compiling HLSL shader: ${FILE_NAME} (${SHADER_PROFILE})"
        )

        list(APPEND HLSL_SPIRV_FILES ${SPIRV})
    endforeach()
endif()

# Create separate targets
add_custom_target(ShadersGLSL DEPENDS ${GLSL_SPIRV_FILES})
add_custom_target(ShadersHLSL DEPENDS ${HLSL_SPIRV_FILES})

# Master target that builds both
add_custom_target(Shaders ALL DEPENDS ShadersGLSL ShadersHLSL)