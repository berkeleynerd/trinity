# Copyright © 2021 CCP ehf.

cmake_minimum_required(VERSION 3.16)

cmake_policy(SET CMP0111 NEW)

if(BUILD_FOR_PYTHON_2)

    set(BUILD_DX11 ON)
    set(BUILD_DX12 ON)
    set(BUILD_METAL ON)
    set(BUILD_SHADER_COMPILER ON)
    set(INSTALL_TO_MONOLITH ON)

else()
    # Feature options
    option(BUILD_SHADER_COMPILER "Build shader compiler." OFF)
    if(WIN32)
        option(BUILD_DX11 "Build dx11." OFF)
        option(BUILD_DX12 "Build dx12." OFF)
    elseif(APPLE)
        option(BUILD_METAL "Build Metal." OFF)
    endif()

    if (BUILD_DX11)
        list(APPEND VCPKG_MANIFEST_FEATURES "dx11")
    endif()

    if (BUILD_DX12)
        list(APPEND VCPKG_MANIFEST_FEATURES "dx12")
    endif()

    if (BUILD_METAL)
        list(APPEND VCPKG_MANIFEST_FEATURES "metal")
    endif()

    if (BUILD_SHADER_COMPILER)
        list(APPEND VCPKG_MANIFEST_FEATURES "shader-compiler")
    endif()

    set(WITH_GRANNY_DEFAULT_FLAG OFF)

    option(WITH_GRANNY "Compile with Granny for .gr2 file support" ${WITH_GRANNY_DEFAULT_FLAG})

    if (WITH_GRANNY)
        list(APPEND VCPKG_MANIFEST_FEATURES "with-granny")
    endif()

    if(APPLE AND BUILD_METAL)
        set(BUILD_GLTF_IMPORTER_DEFAULT ON)
        set(BUILD_ANIMATED_MODEL_SAMPLE_DEFAULT ON)
        set(BUILD_EVE_SCENE_PROBE_DEFAULT ON)
    else()
        set(BUILD_GLTF_IMPORTER_DEFAULT OFF)
        set(BUILD_ANIMATED_MODEL_SAMPLE_DEFAULT OFF)
        set(BUILD_EVE_SCENE_PROBE_DEFAULT OFF)
    endif()

    option(BUILD_GLTF_IMPORTER "Build the glTF to CMF importer." ${BUILD_GLTF_IMPORTER_DEFAULT})
    option(BUILD_ANIMATED_MODEL_SAMPLE "Build the macOS TrinityAL animated model sample." ${BUILD_ANIMATED_MODEL_SAMPLE_DEFAULT})
    option(BUILD_EVE_SCENE_PROBE "Build the macOS EVE scene render driver probe sample." ${BUILD_EVE_SCENE_PROBE_DEFAULT})
    option(BUILD_DESTINY_INTEGRATION "Link the embedded Destiny ballpark contract into Trinity." OFF)

    if(BUILD_GLTF_IMPORTER OR BUILD_ANIMATED_MODEL_SAMPLE OR BUILD_EVE_SCENE_PROBE)
        list(APPEND VCPKG_MANIFEST_FEATURES "gltf-importer")
    endif()

endif()

if(APPLE AND BUILD_METAL)
    project(carbon-trinity VERSION 1.0.0 LANGUAGES C CXX OBJCXX)
else()
    project(carbon-trinity VERSION 1.0.0)
endif()

if(APPLE AND BUILD_METAL)
    set(BUILD_HELLO_TRIANGLE_DEFAULT ON)
else()
    set(BUILD_HELLO_TRIANGLE_DEFAULT OFF)
endif()
option(BUILD_HELLO_TRIANGLE "Build the macOS TrinityAL hello triangle sample." ${BUILD_HELLO_TRIANGLE_DEFAULT})
option(BUILD_FROXEL_GPU_LAB "Build hazardous, explicitly gated Metal froxel lab binaries." OFF)

if(BUILD_FROXEL_GPU_LAB AND NOT (APPLE AND BUILD_METAL))
    message(FATAL_ERROR "BUILD_FROXEL_GPU_LAB requires the macOS Metal backend")
endif()

if (BUILD_FOR_PYTHON_2)
    if(NOT DEFINED ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
        message(FATAL_ERROR "Missing required environment variable CCP_EVE_PERFORCE_BRANCH_PATH")
    elseif(NOT IS_ABSOLUTE $ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
        message(FATAL_ERROR "CCP_EVE_PERFORCE_BRANCH_PATH needs to be an absolute path")
    elseif(NOT EXISTS $ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
        message(FATAL_ERROR "CCP_EVE_PERFORCE_BRANCH_PATH is pointing at a non-existing location $ENV{CCP_EVE_PERFORCE_BRANCH_PATH}")
    elseif(NOT IS_DIRECTORY $ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
        message(FATAL_ERROR "CCP_EVE_PERFORCE_BRANCH_PATH needs to be a directory")
    else()
        message(STATUS "Using CCP_EVE_PERFORCE_BRANCH_PATH located at $ENV{CCP_EVE_PERFORCE_BRANCH_PATH}")
    endif()

    file(TO_CMAKE_PATH "$ENV{CCP_EVE_PERFORCE_BRANCH_PATH}" BRANCH_ROOT_DIR)
    list(INSERT CMAKE_MODULE_PATH 0 ${BRANCH_ROOT_DIR}/cmake)
    include(CcpGlobalSettings)
    include(CcpPackageConfigHelpers)
else()
    include(cmake/CcpTargetConfigurations.cmake)
    include(cmake/CcpDocsGenerator.cmake)
    include(cmake/CcpPackageConfigHelpers.cmake)
    include(cmake/CcpBuildConfigurations.cmake)
endif()

function(ccp_enable_vs_analizers target)
    set_target_properties(${target} PROPERTIES
        VS_GLOBAL_RunCodeAnalysis false
        VS_GLOBAL_EnableMicrosoftCodeAnalysis false
        VS_GLOBAL_EnableClangTidyCodeAnalysis true
    )
endfunction()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(BUILD_FOR_PYTHON_2)
    set(CMAKE_COMPILE_WARNING_AS_ERROR ON) 
endif ()

if (BUILD_SHADER_COMPILER OR BUILD_FOR_PYTHON_2)
    add_subdirectory(shadercompiler)
endif ()

add_subdirectory(trinityal)
if(BUILD_GLTF_IMPORTER OR BUILD_ANIMATED_MODEL_SAMPLE OR BUILD_EVE_SCENE_PROBE)
    add_subdirectory(tools/gltf_to_cmf)
endif()
if(BUILD_EVE_SCENE_PROBE)
    add_subdirectory(tools/dds_to_rgba)
    add_subdirectory(tools/shared_cache_to_gltf)
endif()
if(BUILD_HELLO_TRIANGLE)
    add_subdirectory(samples/hello_triangle)
endif()
if(BUILD_ANIMATED_MODEL_SAMPLE)
    add_subdirectory(samples/animated_model)
endif()
add_subdirectory(trinity)
if(BUILD_EVE_SCENE_PROBE)
    add_subdirectory(samples/eve_scene_probe)
endif()
if(BUILD_FROXEL_GPU_LAB)
    add_subdirectory(tools/froxel_lab)
endif()
