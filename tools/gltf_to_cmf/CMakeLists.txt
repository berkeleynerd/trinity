# Copyright (c) 2026 CCP Games

cmake_minimum_required(VERSION 3.16)

project(TrinityGltfToCmf VERSION 1.0.0)

find_package(CarbonMesh CONFIG REQUIRED)
find_package(draco CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(WebP CONFIG REQUIRED)
find_path(CGLTF_INCLUDE_DIR cgltf.h REQUIRED)

ccp_add_library(TrinityGltfImporter STATIC
    GltfToCmf.cpp
    GltfToCmf.h
)

target_include_directories(TrinityGltfImporter
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CGLTF_INCLUDE_DIR}
        ${Stb_INCLUDE_DIR}
)

target_link_libraries(TrinityGltfImporter
    PUBLIC
        CarbonMesh
    PRIVATE
        draco::draco
        WebP::webp
        WebP::webpdecoder
)

set_target_properties(TrinityGltfImporter
    PROPERTIES
        FOLDER Trinity/Tools
)

ccp_add_executable(TrinityGltfToCmf
    GltfToCmfMain.cpp
)

target_link_libraries(TrinityGltfToCmf
    PRIVATE
        TrinityGltfImporter
)

set_target_properties(TrinityGltfToCmf
    PROPERTIES
        FOLDER Trinity/Tools
)
