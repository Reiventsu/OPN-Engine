file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/GLSL/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/GLSL/*.vert"
)

message(STATUS "Found shader files: ${GLSL_SOURCE_FILES}")

find_program(GLSLC glslc)
if(NOT GLSLC)
    message(FATAL_ERROR "glslc not found! Please install Vulkan SDK")
endif ()

message(STATUS "glslc found at: ${GLSLC}")

set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/Shaders")
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

foreach (GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${SHADER_OUTPUT_DIR}/${FILE_NAME}.spv")

    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${GLSLC} ${GLSL} -o ${SPIRV}
            DEPENDS ${GLSL}
            COMMENT "Compiling shader: ${FILE_NAME}"
    )

    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach ()

add_custom_target(Shaders ALL DEPENDS ${SPIRV_BINARY_FILES})