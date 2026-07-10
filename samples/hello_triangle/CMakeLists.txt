# Copyright © 2026 CCP ehf.

cmake_minimum_required(VERSION 3.16)

if(NOT APPLE)
    return()
endif()

if(NOT BUILD_METAL)
    return()
endif()

project(TrinityALHelloTriangle VERSION 1.0.0)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(XCRUN_TOOL xcrun REQUIRED)

set(GENERATED_SOURCES_DIR ${CMAKE_CURRENT_BINARY_DIR}/GeneratedSources)
set(BIN2H_TOOL ${CMAKE_SOURCE_DIR}/cmake/bin2h.py)

function(add_hello_triangle_shader source output)
    set(shader_source ${CMAKE_CURRENT_SOURCE_DIR}/${source})
    set(shader_binary ${GENERATED_SOURCES_DIR}/${output}.bin)
    set(shader_header ${GENERATED_SOURCES_DIR}/${output}.h)

    add_custom_command(
        OUTPUT ${shader_header}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_SOURCES_DIR}
        COMMAND ${XCRUN_TOOL} metal -x metal -gline-tables-only -MO ${shader_source} -o ${shader_binary}
        COMMAND ${Python3_EXECUTABLE} ${BIN2H_TOOL} ${shader_binary} ${shader_header}
        DEPENDS ${shader_source} ${BIN2H_TOOL}
        VERBATIM
    )

    set(HELLO_TRIANGLE_SHADER_HEADERS ${HELLO_TRIANGLE_SHADER_HEADERS} ${shader_header} PARENT_SCOPE)
endfunction()

add_hello_triangle_shader(PositionOnly.vsh PositionOnly.vs)
add_hello_triangle_shader(ConstantColor.psh ConstantColor.ps)

ccp_add_executable(TrinityALHelloTriangle_metal
    HelloTriangle.mm
    PositionOnly.vsh
    ConstantColor.psh
    ${HELLO_TRIANGLE_SHADER_HEADERS}
)

target_compile_definitions(TrinityALHelloTriangle_metal
    PRIVATE
        TRINITY_PLATFORM=TRINITY_METAL
        TRINITY_AL_WITH_BLUE_EXPOSURE=0
)

target_include_directories(TrinityALHelloTriangle_metal
    PRIVATE
        ${CMAKE_SOURCE_DIR}/trinityal
        ${GENERATED_SOURCES_DIR}
)

target_link_libraries(TrinityALHelloTriangle_metal
    PRIVATE
        TrinityAL_metal
        "-framework Cocoa"
        "-framework QuartzCore"
)

target_compile_options(TrinityALHelloTriangle_metal PRIVATE "-fobjc-arc")
set_target_properties(TrinityALHelloTriangle_metal
    PROPERTIES
        FOLDER Trinity/Samples
        LINKER_LANGUAGE OBJCXX
)

add_custom_target(run_TrinityALHelloTriangle_metal
    COMMAND $<TARGET_FILE:TrinityALHelloTriangle_metal> --frames 120
    DEPENDS TrinityALHelloTriangle_metal
    WORKING_DIRECTORY $<TARGET_FILE_DIR:TrinityALHelloTriangle_metal>
    USES_TERMINAL
)
