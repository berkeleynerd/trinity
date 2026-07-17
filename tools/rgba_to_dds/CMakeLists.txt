# Copyright (c) 2026 CCP Games

cmake_minimum_required(VERSION 3.16)

project(TrinityRgbaToDds VERSION 1.0.0)

find_package(carbon-core CONFIG REQUIRED)
find_package(carbon-imageio CONFIG REQUIRED)

ccp_add_executable(TrinityRgbaToDds
    RgbaToDds.cpp
)

target_link_libraries(TrinityRgbaToDds
    PRIVATE
        CcpImageIO
)

set_target_properties(TrinityRgbaToDds
    PROPERTIES
        FOLDER Trinity/Tools
)
