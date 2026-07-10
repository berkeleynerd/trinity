# Copyright (c) 2026 CCP Games

cmake_minimum_required(VERSION 3.16)

project(TrinitySharedCacheToGltf VERSION 1.0.0)

find_program(NODE_TOOL node REQUIRED)
find_program(NPM_TOOL npm REQUIRED)

set(TRINITY_SHARED_CACHE_TOOL_DIR ${CMAKE_CURRENT_BINARY_DIR}/Runtime)
set(TRINITY_SHARED_CACHE_TOOL_SCRIPT ${TRINITY_SHARED_CACHE_TOOL_DIR}/SharedCacheToGltf.mjs)
set(TRINITY_SHARED_CACHE_TOOL_STAMP ${TRINITY_SHARED_CACHE_TOOL_DIR}/node_modules.stamp)

add_custom_command(
    OUTPUT
        ${TRINITY_SHARED_CACHE_TOOL_SCRIPT}
        ${TRINITY_SHARED_CACHE_TOOL_STAMP}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${TRINITY_SHARED_CACHE_TOOL_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/SharedCacheToGltf.mjs ${TRINITY_SHARED_CACHE_TOOL_SCRIPT}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/package.json ${TRINITY_SHARED_CACHE_TOOL_DIR}/package.json
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/package-lock.json ${TRINITY_SHARED_CACHE_TOOL_DIR}/package-lock.json
    COMMAND ${NPM_TOOL} ci --prefix ${TRINITY_SHARED_CACHE_TOOL_DIR} --ignore-scripts --no-audit --no-fund
    COMMAND ${CMAKE_COMMAND} -E touch ${TRINITY_SHARED_CACHE_TOOL_STAMP}
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/SharedCacheToGltf.mjs
        ${CMAKE_CURRENT_SOURCE_DIR}/package.json
        ${CMAKE_CURRENT_SOURCE_DIR}/package-lock.json
    VERBATIM
)

add_custom_target(TrinitySharedCacheToGltf
    DEPENDS
        ${TRINITY_SHARED_CACHE_TOOL_SCRIPT}
        ${TRINITY_SHARED_CACHE_TOOL_STAMP}
)

set(TRINITY_SHARED_CACHE_NODE_TOOL ${NODE_TOOL} CACHE INTERNAL "Node.js executable for SharedCache conversion")
set(TRINITY_SHARED_CACHE_TO_GLTF_SCRIPT ${TRINITY_SHARED_CACHE_TOOL_SCRIPT} CACHE INTERNAL "Installed SharedCache conversion script")
