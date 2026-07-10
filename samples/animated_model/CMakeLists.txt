# Copyright (c) 2026 CCP Games

cmake_minimum_required(VERSION 3.16)

if(NOT APPLE)
    return()
endif()

if(NOT BUILD_METAL)
    return()
endif()

project(TrinityALAnimatedModel VERSION 1.0.0)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(CarbonMesh CONFIG REQUIRED)
find_program(XCRUN_TOOL xcrun REQUIRED)

set(GENERATED_SOURCES_DIR ${CMAKE_CURRENT_BINARY_DIR}/GeneratedSources)
set(BIN2H_TOOL ${CMAKE_SOURCE_DIR}/cmake/bin2h.py)
set(FOX_SOURCE_GLB ${CMAKE_CURRENT_SOURCE_DIR}/Assets/Fox/Fox.glb)
set(BOX_SOURCE_GLTF ${CMAKE_CURRENT_SOURCE_DIR}/Assets/Box/Box.gltf)
set(FOX_GENERATED_CMF ${CMAKE_CURRENT_BINARY_DIR}/Assets/Fox.cmf)
set(BOX_GENERATED_CMF ${CMAKE_CURRENT_BINARY_DIR}/Assets/Box.cmf)
set(FOX_GENERATED_BASECOLOR_RGBA ${FOX_GENERATED_CMF}.basecolor.rgba)
set(FOX_GENERATED_BASECOLOR_METADATA ${FOX_GENERATED_CMF}.basecolor.txt)

function(add_animated_model_shader source output)
    set(shader_source ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/${source})
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

    set(ANIMATED_MODEL_SHADER_HEADERS ${ANIMATED_MODEL_SHADER_HEADERS} ${shader_header} PARENT_SCOPE)
endfunction()

add_animated_model_shader(PositionColor.vsh PositionColor.vs)
add_animated_model_shader(PositionColor.psh PositionColor.ps)

function(add_animated_model_asset output source)
    add_custom_command(
        OUTPUT ${output}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/Assets
        COMMAND $<TARGET_FILE:TrinityGltfToCmf> --input ${source} --output ${output} --summary
        DEPENDS TrinityGltfToCmf ${source}
        VERBATIM
    )
endfunction()

add_animated_model_asset(${FOX_GENERATED_CMF} ${FOX_SOURCE_GLB})
add_animated_model_asset(${BOX_GENERATED_CMF} ${BOX_SOURCE_GLTF})

add_custom_target(TrinityAnimatedModelAssets
    DEPENDS
        ${FOX_GENERATED_CMF}
        ${BOX_GENERATED_CMF}
)

ccp_add_executable(TrinityALAnimatedModel_metal
    AnimatedModel.mm
    Shaders/PositionColor.vsh
    Shaders/PositionColor.psh
    ${ANIMATED_MODEL_SHADER_HEADERS}
)

add_dependencies(TrinityALAnimatedModel_metal TrinityAnimatedModelAssets)

target_compile_definitions(TrinityALAnimatedModel_metal
    PRIVATE
        TRINITY_PLATFORM=TRINITY_METAL
        TRINITY_AL_WITH_BLUE_EXPOSURE=0
)

target_include_directories(TrinityALAnimatedModel_metal
    PRIVATE
        ${CMAKE_SOURCE_DIR}/trinityal
        ${CMAKE_SOURCE_DIR}/trinityal/include
        ${GENERATED_SOURCES_DIR}
)

target_link_libraries(TrinityALAnimatedModel_metal
    PRIVATE
        TrinityAL_metal
        CarbonMesh
        "-framework Cocoa"
        "-framework QuartzCore"
)

target_compile_options(TrinityALAnimatedModel_metal PRIVATE "-fobjc-arc")
set_target_properties(TrinityALAnimatedModel_metal
    PROPERTIES
        FOLDER Trinity/Samples
        LINKER_LANGUAGE OBJCXX
)

add_custom_command(
    TARGET TrinityALAnimatedModel_metal
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TrinityALAnimatedModel_metal>/Assets
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FOX_GENERATED_CMF} $<TARGET_FILE_DIR:TrinityALAnimatedModel_metal>/Assets/Fox.cmf
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FOX_GENERATED_BASECOLOR_RGBA} $<TARGET_FILE_DIR:TrinityALAnimatedModel_metal>/Assets/Fox.cmf.basecolor.rgba
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FOX_GENERATED_BASECOLOR_METADATA} $<TARGET_FILE_DIR:TrinityALAnimatedModel_metal>/Assets/Fox.cmf.basecolor.txt
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BOX_GENERATED_CMF} $<TARGET_FILE_DIR:TrinityALAnimatedModel_metal>/Assets/Box.cmf
    VERBATIM
)

add_custom_target(TrinityAnimatedModelRuntimeAssets
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TrinityALAnimatedModel_metal>/Assets
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FOX_GENERATED_CMF} $<TARGET_FILE_DIR:TrinityALAnimatedModel_metal>/Assets/Fox.cmf
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FOX_GENERATED_BASECOLOR_RGBA} $<TARGET_FILE_DIR:TrinityALAnimatedModel_metal>/Assets/Fox.cmf.basecolor.rgba
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FOX_GENERATED_BASECOLOR_METADATA} $<TARGET_FILE_DIR:TrinityALAnimatedModel_metal>/Assets/Fox.cmf.basecolor.txt
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BOX_GENERATED_CMF} $<TARGET_FILE_DIR:TrinityALAnimatedModel_metal>/Assets/Box.cmf
    DEPENDS
        TrinityALAnimatedModel_metal
        TrinityAnimatedModelAssets
    VERBATIM
)

add_custom_target(run_TrinityALAnimatedModel_metal
    COMMAND $<TARGET_FILE:TrinityALAnimatedModel_metal> --frames 180
    DEPENDS TrinityAnimatedModelRuntimeAssets
    WORKING_DIRECTORY $<TARGET_FILE_DIR:TrinityALAnimatedModel_metal>
    USES_TERMINAL
)

add_custom_target(run_TrinityALAnimatedModelBox_metal
    COMMAND $<TARGET_FILE:TrinityALAnimatedModel_metal> --asset Box --frames 180
    DEPENDS TrinityAnimatedModelRuntimeAssets
    WORKING_DIRECTORY $<TARGET_FILE_DIR:TrinityALAnimatedModel_metal>
    USES_TERMINAL
)
