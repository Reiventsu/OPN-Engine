find_program(SLANG_COMPILER slangc REQUIRED)

file(GLOB SHADER_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.slang" "*.hlsl" "*.glsl")

set(SPIRV_OUTPUTS "")
set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/Shaders")

file(MAKE_DIRECTORY ${OUTPUT_DIR})

foreach (SHADER_FILE ${SHADER_SOURCES})
    if(NOT SHADER_FILE MATCHES "\\.(vert|frag|comp)\\.")
        CONTINUE()
    endif()

    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME_WLE)
    set(OUTPUT_FILE "${OUTPUT_DIR}/${SHADER_NAME}.spv")
    set(DEP_FILE "${OUTPUT_DIR}/${SHADER_NAME}.d")

    set(STAGE_FLAG "")
    if(SHADER_FILE MATCHES ".*\\.comp\\..*")
        set(STAGE_FLAG "-stage;compute")
    elseif(SHADER_FILE MATCHES ".*\\.vert\\..*")
        set(STAGE_FLAG "-stage;vertex")
    elseif(SHADER_FILE MATCHES ".*\\.frag\\..*")
        set(STAGE_FLAG "-stage;fragment")
    endif()

    add_custom_command(
            OUTPUT ${OUTPUT_FILE}
            DEPFILE ${DEP_FILE}
            COMMAND ${SLANG_COMPILER} "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_FILE}"
            -target spirv
            -entry main
            ${STAGE_FLAG}
            -o "${OUTPUT_FILE}"
            -depfile "${DEP_FILE}"
            -I "${CMAKE_CURRENT_SOURCE_DIR}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_FILE}"
            COMMENT "Compiling shader: ${SHADER_FILE}"
            VERBATIM
    )

    list(APPEND SPIRV_OUTPUTS ${OUTPUT_FILE})
endforeach ()

add_custom_target(CompileShaders ALL DEPENDS ${SPIRV_OUTPUTS})