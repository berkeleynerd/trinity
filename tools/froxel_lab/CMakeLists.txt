# Copyright (c) 2026 CCP ehf.

cmake_minimum_required(VERSION 3.16)

ccp_add_executable(TrinityALFroxelProbe_metal EXCLUDE_FROM_ALL FroxelProbe.mm)
target_link_libraries(TrinityALFroxelProbe_metal PRIVATE
    TrinityAL_metal
    "-framework Foundation"
    "-framework Metal"
)
target_include_directories(TrinityALFroxelProbe_metal PRIVATE
    ${CMAKE_SOURCE_DIR}/trinityal
    ${CMAKE_SOURCE_DIR}/trinityal/include
    ${CMAKE_SOURCE_DIR}/trinityal/metal
)
target_compile_definitions(TrinityALFroxelProbe_metal PRIVATE
    TRINITY_AL_WITH_BLUE_EXPOSURE=0
    TRINITY_FROXEL_LAB_TOOL="${CMAKE_CURRENT_SOURCE_DIR}/froxel_lab.py"
)
target_compile_options(TrinityALFroxelProbe_metal PRIVATE
    "-fobjc-arc"
    "-Wno-nonportable-include-path"
)

if(BUILD_EVE_SCENE_PROBE)
    ccp_add_executable(TrinityALEveSceneProbeFroxelLab_metal EXCLUDE_FROM_ALL
        ${CMAKE_SOURCE_DIR}/samples/eve_scene_probe/EveSceneProbe.mm
    )
    target_link_libraries(TrinityALEveSceneProbeFroxelLab_metal PRIVATE
        trinity_metal
        "-framework Cocoa"
        "-framework ImageIO"
        "-framework QuartzCore"
        "-framework UniformTypeIdentifiers"
    )
    target_include_directories(TrinityALEveSceneProbeFroxelLab_metal PRIVATE
        ${CMAKE_SOURCE_DIR}/trinity
        ${CMAKE_SOURCE_DIR}/trinity/include
        ${CMAKE_SOURCE_DIR}/trinityal
        ${CMAKE_SOURCE_DIR}/trinityal/include
    )
    target_compile_definitions(TrinityALEveSceneProbeFroxelLab_metal PRIVATE
        TRINITY_PLATFORM=TRINITY_METAL
        TRINITY_AL_WITH_BLUE_EXPOSURE=1
        BLUE_OVERRIDE_VECTOR_TYPES=1
        TBB_ENABLED=1
        DEPRECATED_ENABLED=1
        TRINITY_FROXEL_INCIDENT_LAB=1
        TRINITY_FROXEL_LAB_TOOL="${CMAKE_CURRENT_SOURCE_DIR}/froxel_lab.py"
    )
    target_compile_options(TrinityALEveSceneProbeFroxelLab_metal PRIVATE
        "-fobjc-arc"
        "-fdeclspec"
        "-Wno-nonportable-include-path"
        "-Wno-inconsistent-missing-override"
    )
    set_target_properties(TrinityALEveSceneProbeFroxelLab_metal PROPERTIES
        FOLDER Trinity/Lab
        LINKER_LANGUAGE OBJCXX
        BUILD_RPATH "$<TARGET_FILE_DIR:trinity_metal>;${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/lib"
    )
    if(TARGET TrinityEveSceneProbeAssets)
        add_dependencies(TrinityALEveSceneProbeFroxelLab_metal TrinityEveSceneProbeAssets)
    endif()
    add_custom_command(TARGET TrinityALEveSceneProbeFroxelLab_metal POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_BINARY_DIR}/samples/eve_scene_probe/Assets
            $<TARGET_FILE_DIR:TrinityALEveSceneProbeFroxelLab_metal>/Assets
        VERBATIM
    )
endif()
