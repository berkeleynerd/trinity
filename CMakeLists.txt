project(trinity VERSION 1.0.0)

find_package(muparser REQUIRED)
find_package(libSquish REQUIRED)
find_package(FidelityFX REQUIRED)
find_package(Granny REQUIRED)
find_package(IntelTBB REQUIRED)

# utility function that sets properties shared between all trinity targets
function(set_shared_trinity_properties target)
    target_link_libraries(${target}
        PRIVATE
            ImageIO
            Blue
            Freetype::Freetype
            muParser
            SQUISH::SQUISH
            AMD::FidelityFX
            RadGameTools::Granny
            Intel::TBB
            CcpMath
            PDM::PDM
    )

    # FIXME we are only tolerating this due to a somewhat high velocity
    # From the Cmake documentation:
    # Note: We do not recommend using GLOB to collect a list of source files from your source tree.
    # If no CMakeLists.txt file changes when a source is added or removed then the generated build
    # system cannot know when to ask CMake to regenerate.
    file(GLOB_RECURSE _SOURCES "*.cpp" "*.mm" "*.h")
    target_sources(${target} PRIVATE ${_SOURCES})
    source_group(
        TREE "${CMAKE_CURRENT_SOURCE_DIR}"
        PREFIX "Sources"
        FILES ${_SOURCES}
    )

    target_precompile_headers(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/StdAfx.h)
    target_include_directories(${target}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/..
    )
    target_compile_definitions(${target}
        PRIVATE
            BLUE_OVERRIDE_VECTOR_TYPES=1
            TRINITYBUILD
            TRINITY_AL_WITH_BLUE_EXPOSURE=1
            TBB_ENABLED=1
            DEPRECATED_ENABLED=1
    )

    # override the output directory to ensure that all TrinityAL outputs end up in the same folder.
    # this is OK because we're differentiating them by OUTPUT_NAME
    set(output_directory "${CMAKE_SOURCE_DIR}/carbon/autobuild/Trinity/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/$<$<CONFIG:DEBUG>:>")
    message(STATUS "Overriding ${target} output directory to ${output_directory}")
    set_target_properties(${target}
        PROPERTIES
            FOLDER Trinity
            CXX_VISIBILITY_PRESET hidden
            ARCHIVE_OUTPUT_DIRECTORY "${output_directory}"
            LIBRARY_OUTPUT_DIRECTORY "${output_directory}"
            RUNTIME_OUTPUT_DIRECTORY "${output_directory}"
            PDB_OUTPUT_DIRECTORY ${output_directory}
            OUTPUT_NAME "_${target}"
    )
endfunction()

#[[
FIXME currently the _stub platform does not build on macOS due to "unknown type name: 'CGKeyCode'"
There are probably more issues because we haven't build the stub project on macOS so far IIRC
]]
if(WIN32)
add_library(trinity_stub SHARED EXCLUDE_FROM_ALL)
target_compile_definitions(trinity_stub
    PRIVATE
        TRINITY_PLATFORM=TRINITY_STUB
        TRINITYNAME=_trinity_stub
)
target_link_libraries(trinity_stub PRIVATE TrinityAL_stub)
set_shared_trinity_properties(trinity_stub)
endif()

if(WIN32)
    add_library(trinity_dx9 SHARED EXCLUDE_FROM_ALL)
    target_compile_definitions(trinity_dx9
        PRIVATE
            TRINITY_PLATFORM=TRINITY_DIRECTX9
            TRINITYNAME=_trinity_dx9
    )
    target_link_libraries(trinity_dx9 PRIVATE TrinityAL_dx9)
    set_shared_trinity_properties(trinity_dx9)

    add_library(trinity_dx11 SHARED)
    target_compile_definitions(trinity_dx11
        PRIVATE
            TRINITY_PLATFORM=TRINITY_DIRECTX11
            TRINITYNAME=_trinity_dx11
    )
    target_link_libraries(trinity_dx11 PRIVATE TrinityAL_dx11)
    set_shared_trinity_properties(trinity_dx11)

    add_library(trinity_dx12 SHARED EXCLUDE_FROM_ALL)
    target_compile_definitions(trinity_dx12
        PRIVATE
            TRINITY_PLATFORM=TRINITY_DIRECTX12
            TRINITYNAME=_trinity_dx12
    )
    target_link_libraries(trinity_dx12 PRIVATE TrinityAL_dx12)
    set_shared_trinity_properties(trinity_dx12)
elseif(APPLE)
    enable_language(OBJCXX)
    add_library(trinity_metal SHARED)
    target_compile_definitions(trinity_metal
        PRIVATE
            TRINITY_PLATFORM=TRINITY_METAL
            TRINITYNAME=_trinity_metal
    )
    set_shared_trinity_properties(trinity_metal)
    target_link_libraries(trinity_metal
        PRIVATE
            TrinityAL_metal
            "-framework CoreFoundation"
            "-framework Cocoa"
            "-framework Carbon"
            "-framework Metal"
            "-framework IOKit"
    )

    #[[
        FIXME because we're mixing objective-c++ and c++ headers in our precompiled headers then we need to force
        one specific language for macOS. This can be fixed once we revisit our usage of precompiled headers, which is
        still very MSVC specific.
    ]]
    file(GLOB_RECURSE _CPP_SOURCES "*.cpp" "*.cxx")
    set_source_files_properties(${_CPP_SOURCES} PROPERTIES LANGUAGE OBJCXX)
    set_source_files_properties(UI/Scancodes.mm PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

    target_compile_options(trinity_metal
        PRIVATE
            "-fobjc-arc"
            # disabling these warnings is temporary so that we don't drown in them
            "-Wno-nonportable-include-path"
            "-Wno-inconsistent-missing-override"
    )

    set_target_properties(trinity_metal
        PROPERTIES
            LINKER_LANGUAGE OBJCXX
            PREFIX ""
            SUFFIX ".so"
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH "@executable_path/."
    )
else()
    message(FATAL_ERROR "Unsupported trinity platform, only stub would be available!")
endif()
