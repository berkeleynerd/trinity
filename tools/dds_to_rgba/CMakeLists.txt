# Copyright (c) 2026 CCP Games

cmake_minimum_required(VERSION 3.16)

project(TrinityDdsToRgba VERSION 1.0.0)

include(FetchContent)

FetchContent_Declare(bcdec
    URL https://github.com/iOrange/bcdec/archive/93628fe5627102fe5187b7eeb99122dec6612c36.tar.gz
    URL_HASH SHA256=1555c951cdc86f41d79f31e4558d3f89e97f8462c61ad56d3475a00c6d214195
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR do-not-add-upstream-test-target
)
FetchContent_MakeAvailable(bcdec)

find_package(carbon-core CONFIG REQUIRED)
find_package(carbon-imageio CONFIG REQUIRED)

ccp_add_executable(TrinityDdsToRgba
    DdsToRgba.cpp
)

target_include_directories(TrinityDdsToRgba
    PRIVATE
        ${bcdec_SOURCE_DIR}
)

target_link_libraries(TrinityDdsToRgba
    PRIVATE
        CcpImageIO
)

set_target_properties(TrinityDdsToRgba
    PROPERTIES
        FOLDER Trinity/Tools
)
