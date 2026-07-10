# Copyright (c) 2026 CCP Games

cmake_minimum_required(VERSION 3.16)

if(NOT APPLE)
    return()
endif()

if(NOT BUILD_METAL)
    return()
endif()

project(TrinityALEveSceneProbe VERSION 1.0.0)

find_package(carbon-blue CONFIG REQUIRED)
find_package(carbon-math CONFIG REQUIRED)

set(EVE_SCENE_PROBE_ASSETS_DIR ${CMAKE_CURRENT_BINARY_DIR}/Assets)
set(EVE_SCENE_PROBE_SHIP_SOURCE "$ENV{HOME}/Downloads/11989_lite.glb")
set(EVE_SCENE_PROBE_SHIP_CMF ${EVE_SCENE_PROBE_ASSETS_DIR}/11989_lite.cmf)
set(EVE_SCENE_PROBE_SHIP_BASECOLOR_RGBA ${EVE_SCENE_PROBE_SHIP_CMF}.basecolor.rgba)
set(EVE_SCENE_PROBE_SHIP_BASECOLOR_METADATA ${EVE_SCENE_PROBE_SHIP_CMF}.basecolor.txt)
set(EVE_SCENE_PROBE_SHARED_CACHE "$ENV{HOME}/Library/Application Support/EVE Online/SharedCache" CACHE PATH "EVE SharedCache used by the scene probe")
set(EVE_SCENE_PROBE_SHARED_CACHE_INDEX ${EVE_SCENE_PROBE_SHARED_CACHE}/tq/EVE.app/Contents/Resources/build/resfileindex.txt)
set(EVE_SCENE_PROBE_SHARED_CACHE_MACOS_INDEX ${EVE_SCENE_PROBE_SHARED_CACHE}/tq/EVE.app/Contents/Resources/build/resfileindex_macOS.txt)
set(EVE_SCENE_PROBE_ASTERO_MODEL "res:/dx9/model/ship/soe/frigate/soef1/soef1_t1.gr2")
set(EVE_SCENE_PROBE_ASTERO_BASE_COLOR "res:/dx9/model/ship/soe/frigate/soef1/soef1_t1_a.dds")
set(EVE_SCENE_PROBE_ASTERO_NORMAL "res:/dx9/model/ship/soe/frigate/soef1/soef1_t1_n.dds")
set(EVE_SCENE_PROBE_ASTERO_ROUGHNESS "res:/dx9/model/ship/soe/frigate/soef1/soef1_t1_r.dds")
set(EVE_SCENE_PROBE_ASTERO_MATERIAL "res:/dx9/model/ship/soe/frigate/soef1/soef1_t1_m.dds")
set(EVE_SCENE_PROBE_ASTERO_GLOW "res:/dx9/model/ship/soe/frigate/soef1/soef1_t1_g.dds")
set(EVE_SCENE_PROBE_ASTERO_DETAIL "res:/dx9/model/ship/soe/frigate/soef1/soef1_t1_d.dds")
set(EVE_SCENE_PROBE_ASTERO_MASK "res:/dx9/model/ship/soe/frigate/soef1/soef1_t1_mask.dds")
set(EVE_SCENE_PROBE_ASTERO_PAINT "res:/dx9/model/ship/soe/frigate/soef1/soef1_t1_p3.dds")
set(EVE_SCENE_PROBE_ASTERO_GLTF ${EVE_SCENE_PROBE_ASSETS_DIR}/Astero.gltf)
set(EVE_SCENE_PROBE_ASTERO_BIN ${EVE_SCENE_PROBE_ASSETS_DIR}/Astero.bin)
set(EVE_SCENE_PROBE_ASTERO_DDS ${EVE_SCENE_PROBE_ASSETS_DIR}/Astero_basecolor.dds)
set(EVE_SCENE_PROBE_ASTERO_NORMAL_DDS ${EVE_SCENE_PROBE_ASSETS_DIR}/Astero_normal.dds)
set(EVE_SCENE_PROBE_ASTERO_ROUGHNESS_DDS ${EVE_SCENE_PROBE_ASSETS_DIR}/Astero_roughness.dds)
set(EVE_SCENE_PROBE_ASTERO_MATERIAL_DDS ${EVE_SCENE_PROBE_ASSETS_DIR}/Astero_material.dds)
set(EVE_SCENE_PROBE_ASTERO_GLOW_DDS ${EVE_SCENE_PROBE_ASSETS_DIR}/Astero_glow.dds)
set(EVE_SCENE_PROBE_ASTERO_DETAIL_DDS ${EVE_SCENE_PROBE_ASSETS_DIR}/Astero_d.dds)
set(EVE_SCENE_PROBE_ASTERO_MASK_DDS ${EVE_SCENE_PROBE_ASSETS_DIR}/Astero_mask.dds)
set(EVE_SCENE_PROBE_ASTERO_PAINT_DDS ${EVE_SCENE_PROBE_ASSETS_DIR}/Astero_p3.dds)
set(EVE_SCENE_PROBE_ASTERO_CMF ${EVE_SCENE_PROBE_ASSETS_DIR}/Astero.cmf)
set(EVE_SCENE_PROBE_ASTERO_BASECOLOR_RGBA ${EVE_SCENE_PROBE_ASTERO_CMF}.basecolor.rgba)
set(EVE_SCENE_PROBE_ASTERO_BASECOLOR_METADATA ${EVE_SCENE_PROBE_ASTERO_CMF}.basecolor.txt)
set(EVE_SCENE_PROBE_ASTERO_NORMAL_RGBA ${EVE_SCENE_PROBE_ASTERO_CMF}.normal.rgba)
set(EVE_SCENE_PROBE_ASTERO_NORMAL_METADATA ${EVE_SCENE_PROBE_ASTERO_CMF}.normal.txt)
set(EVE_SCENE_PROBE_ASTERO_ROUGHNESS_RGBA ${EVE_SCENE_PROBE_ASTERO_CMF}.roughness.rgba)
set(EVE_SCENE_PROBE_ASTERO_ROUGHNESS_METADATA ${EVE_SCENE_PROBE_ASTERO_CMF}.roughness.txt)
set(EVE_SCENE_PROBE_ASTERO_MATERIAL_RGBA ${EVE_SCENE_PROBE_ASTERO_CMF}.material.rgba)
set(EVE_SCENE_PROBE_ASTERO_MATERIAL_METADATA ${EVE_SCENE_PROBE_ASTERO_CMF}.material.txt)
set(EVE_SCENE_PROBE_ASTERO_GLOW_RGBA ${EVE_SCENE_PROBE_ASTERO_CMF}.glow.rgba)
set(EVE_SCENE_PROBE_ASTERO_GLOW_METADATA ${EVE_SCENE_PROBE_ASTERO_CMF}.glow.txt)
set(EVE_SCENE_PROBE_ASTERO_DETAIL_RGBA ${EVE_SCENE_PROBE_ASTERO_CMF}.d.rgba)
set(EVE_SCENE_PROBE_ASTERO_DETAIL_METADATA ${EVE_SCENE_PROBE_ASTERO_CMF}.d.txt)
set(EVE_SCENE_PROBE_ASTERO_MASK_RGBA ${EVE_SCENE_PROBE_ASTERO_CMF}.mask.rgba)
set(EVE_SCENE_PROBE_ASTERO_MASK_METADATA ${EVE_SCENE_PROBE_ASTERO_CMF}.mask.txt)
set(EVE_SCENE_PROBE_ASTERO_PAINT_RGBA ${EVE_SCENE_PROBE_ASTERO_CMF}.p3.rgba)
set(EVE_SCENE_PROBE_ASTERO_PAINT_METADATA ${EVE_SCENE_PROBE_ASTERO_CMF}.p3.txt)
set(EVE_SCENE_PROBE_EFFECT_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/AsteroPbr.fx)
set(EVE_SCENE_PROBE_RENDER_PRODUCT_EFFECT_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/RenderProductVisualizer.fx)
set(EVE_SCENE_PROBE_EFFECT_DIR ${EVE_SCENE_PROBE_ASSETS_DIR}/graphics/effect.metal/managed/space/spaceobject/probe)
set(EVE_SCENE_PROBE_EFFECT ${EVE_SCENE_PROBE_EFFECT_DIR}/asteropbr.sm_hi)
set(EVE_SCENE_PROBE_EFFECT_DEPTH ${EVE_SCENE_PROBE_EFFECT_DIR}/asteropbr.sm_depth)
set(EVE_SCENE_PROBE_RENDER_PRODUCT_EFFECT ${EVE_SCENE_PROBE_EFFECT_DIR}/renderproductvisualizer.sm_hi)
set(EVE_SCENE_PROBE_RENDER_PRODUCT_EFFECT_DEPTH ${EVE_SCENE_PROBE_EFFECT_DIR}/renderproductvisualizer.sm_depth)
set(EVE_SCENE_PROBE_POSTPROCESS_DIR ${EVE_SCENE_PROBE_ASSETS_DIR}/graphics/effect.metal/managed/space/postprocess)
set(EVE_SCENE_PROBE_SYSTEM_EFFECT_DIR ${EVE_SCENE_PROBE_ASSETS_DIR}/graphics/effect.metal/managed/space/system)
set(EVE_SCENE_PROBE_TONEMAPPING_EFFECT ${EVE_SCENE_PROBE_POSTPROCESS_DIR}/tonemapping.sm_hi)
set(EVE_SCENE_PROBE_TONEMAPPING_EFFECT_DEPTH ${EVE_SCENE_PROBE_POSTPROCESS_DIR}/tonemapping.sm_depth)
set(EVE_SCENE_PROBE_CREATE_HISTOGRAMS_EFFECT ${EVE_SCENE_PROBE_POSTPROCESS_DIR}/createhistograms.sm_hi)
set(EVE_SCENE_PROBE_CREATE_HISTOGRAMS_EFFECT_DEPTH ${EVE_SCENE_PROBE_POSTPROCESS_DIR}/createhistograms.sm_depth)
set(EVE_SCENE_PROBE_MERGE_HISTOGRAMS_EFFECT ${EVE_SCENE_PROBE_POSTPROCESS_DIR}/mergehistograms.sm_hi)
set(EVE_SCENE_PROBE_MERGE_HISTOGRAMS_EFFECT_DEPTH ${EVE_SCENE_PROBE_POSTPROCESS_DIR}/mergehistograms.sm_depth)
set(EVE_SCENE_PROBE_MEASURE_EXPOSURE_EFFECT ${EVE_SCENE_PROBE_POSTPROCESS_DIR}/measureexposure.sm_hi)
set(EVE_SCENE_PROBE_MEASURE_EXPOSURE_EFFECT_DEPTH ${EVE_SCENE_PROBE_POSTPROCESS_DIR}/measureexposure.sm_depth)
set(EVE_SCENE_PROBE_BLIT_EFFECT ${EVE_SCENE_PROBE_SYSTEM_EFFECT_DIR}/blit.sm_hi)
set(EVE_SCENE_PROBE_BLIT_EFFECT_DEPTH ${EVE_SCENE_PROBE_SYSTEM_EFFECT_DIR}/blit.sm_depth)
set(EVE_SCENE_PROBE_BLIT_FILTERED_EFFECT ${EVE_SCENE_PROBE_SYSTEM_EFFECT_DIR}/blitfiltered.sm_hi)
set(EVE_SCENE_PROBE_BLIT_FILTERED_EFFECT_DEPTH ${EVE_SCENE_PROBE_SYSTEM_EFFECT_DIR}/blitfiltered.sm_depth)
set(EVE_SCENE_PROBE_GLOBAL_TEXTURE_DIR ${EVE_SCENE_PROBE_ASSETS_DIR}/texture/global)
set(EVE_SCENE_PROBE_BLACK_TEXTURE ${EVE_SCENE_PROBE_GLOBAL_TEXTURE_DIR}/black.dds)
set(EVE_SCENE_PROBE_REFLECTION_TEXTURE ${EVE_SCENE_PROBE_GLOBAL_TEXTURE_DIR}/genericreflection_cube.dds)
set(EVE_SCENE_PROBE_FLAT_NORMAL_RESOURCE "res:/texture/global/flatnormal.dds")
set(EVE_SCENE_PROBE_FLAT_NORMAL_TEXTURE ${EVE_SCENE_PROBE_GLOBAL_TEXTURE_DIR}/flatnormal.dds)
set(EVE_SCENE_PROBE_REFLECTION_CORRECTION_RESOURCE "res:/texture/reflectioncorrection/128x128.dds")
set(EVE_SCENE_PROBE_REFLECTION_CORRECTION_TEXTURE ${EVE_SCENE_PROBE_ASSETS_DIR}/texture/reflectioncorrection/128x128.dds)
set(EVE_SCENE_PROBE_ASTERO_HULL_BLACK_RESOURCE "res:/dx9/model/spaceobjectfactory/hulls/soef1_t1.black")
set(EVE_SCENE_PROBE_ASTERO_FACTION_BLACK_RESOURCE "res:/dx9/model/spaceobjectfactory/factions/soebase.black")
set(EVE_SCENE_PROBE_ASTERO_MATERIAL_RESOURCES
    "res:/dx9/model/spaceobjectfactory/materials/chrome_metallic.black"
    "res:/dx9/model/spaceobjectfactory/materials/grey_steel_brushed.black"
    "res:/dx9/model/spaceobjectfactory/materials/white_ghost_matt.black"
    "res:/dx9/model/spaceobjectfactory/materials/red_crimson_enamel.black"
    "res:/dx9/model/spaceobjectfactory/materials/glass_soe.black"
)
set(EVE_SCENE_PROBE_QUADV5_HI_RESOURCE "res:/graphics/effect.metal/managed/space/spaceobject/v5/quad/quadv5.sm_hi")
set(EVE_SCENE_PROBE_QUADV5_LO_RESOURCE "res:/graphics/effect.metal/managed/space/spaceobject/v5/quad/quadv5.sm_lo")
set(EVE_SCENE_PROBE_QUADV5_DEPTH_RESOURCE "res:/graphics/effect.metal/managed/space/spaceobject/v5/quad/quadv5.sm_depth")
set(EVE_SCENE_PROBE_QUADHEATV5_HI_RESOURCE "res:/graphics/effect.metal/managed/space/spaceobject/v5/quad/quadheatv5.sm_hi")
set(EVE_SCENE_PROBE_QUADHEATV5_LO_RESOURCE "res:/graphics/effect.metal/managed/space/spaceobject/v5/quad/quadheatv5.sm_lo")
set(EVE_SCENE_PROBE_QUADHEATV5_DEPTH_RESOURCE "res:/graphics/effect.metal/managed/space/spaceobject/v5/quad/quadheatv5.sm_depth")
set(EVE_SCENE_PROBE_FXDISTORTIONV5_HI_RESOURCE "res:/graphics/effect.metal/managed/space/spaceobject/v5/fx/fxdistortionv5.sm_hi")
set(EVE_SCENE_PROBE_FXDISTORTIONV5_LO_RESOURCE "res:/graphics/effect.metal/managed/space/spaceobject/v5/fx/fxdistortionv5.sm_lo")
set(EVE_SCENE_PROBE_FXDISTORTIONV5_DEPTH_RESOURCE "res:/graphics/effect.metal/managed/space/spaceobject/v5/fx/fxdistortionv5.sm_depth")
set(EVE_SCENE_PROBE_WARP_DISTORTION_RESOURCE "res:/texture/sprite/warptunnel4_n.dds")
set(EVE_SCENE_PROBE_FITTING_SCENE_RESOURCE "res:/dx9/scene/fitting/fitting.black")
set(EVE_SCENE_PROBE_FITTING_CUBE_RESOURCE "res:/dx9/scene/fitting/fitting_cube.dds")
set(EVE_SCENE_PROBE_FITTING_CUBE_BLUR_RESOURCE "res:/dx9/scene/fitting/fitting_cube_blur.dds")
set(EVE_SCENE_PROBE_FITTING_CUBE_REFL_RESOURCE "res:/dx9/scene/fitting/fitting_cube_refl.dds")
set(EVE_SCENE_PROBE_DEFAULT_POSTPROCESS_RESOURCE "res:/dx9/default/postprocess.black")
set(EVE_SCENE_PROBE_A01_SCENE_RESOURCE "res:/dx9/scene/universe/a01_cube.black")
set(EVE_SCENE_PROBE_A01_CUBE_RESOURCE "res:/dx9/scene/universe/a01_cube.dds")
set(EVE_SCENE_PROBE_A01_REFLECTION_RESOURCE "res:/dx9/scene/universe/a01_cube_refl.dds")
set(EVE_SCENE_PROBE_A01_BLUR_RESOURCE "res:/dx9/scene/universe/a01_cube_blur.dds")
set(EVE_SCENE_PROBE_A01_LOW_DETAIL_RESOURCE "res:/dx9/scene/universe/a01_cube_lowdetail.dds")
set(EVE_SCENE_PROBE_A01_LOW_QUALITY_RESOURCE "res:/dx9/scene/universe/lowquality/wormhole_a01.dds")
set(EVE_SCENE_PROBE_A01_LOW_QUALITY_MIX_RESOURCE "res:/dx9/scene/universe/lowquality/wormhole_a01_mix.dds")
set(EVE_SCENE_PROBE_A01_OVERLAY_RESOURCE "res:/texture/fx/misc/rgb_01a.dds")
set(EVE_SCENE_PROBE_A01_STARS_RESOURCE "res:/dx9/scene/starfield/stars01_tile2.dds")
set(EVE_SCENE_PROBE_BACKGROUND_HI_RESOURCE "res:/graphics/effect.metal/managed/space/specialfx/background.sm_hi")
set(EVE_SCENE_PROBE_BACKGROUND_LO_RESOURCE "res:/graphics/effect.metal/managed/space/specialfx/background.sm_lo")
set(EVE_SCENE_PROBE_BACKGROUND_DEPTH_RESOURCE "res:/graphics/effect.metal/managed/space/specialfx/background.sm_depth")
set(EVE_SCENE_PROBE_SPRITE_STARS_RESOURCE "res:/dx9/scene/starfield/spritestars.black")
set(EVE_SCENE_PROBE_STAR_ATLAS_RESOURCE "res:/dx9/scene/starfield/staratlas.dds")
set(EVE_SCENE_PROBE_STAR_COLORS_RESOURCE "res:/dx9/scene/starfield/starcolors.dds")
set(EVE_SCENE_PROBE_STAR_SPRITES_HI_RESOURCE "res:/graphics/effect.metal/managed/space/specialfx/starsprites.sm_hi")
set(EVE_SCENE_PROBE_STAR_SPRITES_LO_RESOURCE "res:/graphics/effect.metal/managed/space/specialfx/starsprites.sm_lo")
set(EVE_SCENE_PROBE_STAR_SPRITES_DEPTH_RESOURCE "res:/graphics/effect.metal/managed/space/specialfx/starsprites.sm_depth")
set(EVE_SCENE_PROBE_NEW_EDEN_SUN_RESOURCE "res:/dx9/model/celestial/sun/sun_yellow_small_01b.black")
set(EVE_SCENE_PROBE_NEW_EDEN_PLANET_RESOURCE "res:/dx9/model/worldobject/planet/sandstormplanet.black")
set(EVE_SCENE_PROBE_COMPUTE_LIGHT_LISTS_HI_RESOURCE "res:/graphics/effect.metal/managed/space/system/computelightlists.sm_hi")
set(EVE_SCENE_PROBE_COMPUTE_LIGHT_LISTS_LO_RESOURCE "res:/graphics/effect.metal/managed/space/system/computelightlists.sm_lo")
set(EVE_SCENE_PROBE_COMPUTE_LIGHT_LISTS_DEPTH_RESOURCE "res:/graphics/effect.metal/managed/space/system/computelightlists.sm_depth")
set(EVE_SCENE_PROBE_CLIENT_HULL_DIR ${EVE_SCENE_PROBE_ASSETS_DIR}/dx9/model/spaceobjectfactory/hulls)
set(EVE_SCENE_PROBE_CLIENT_FACTION_DIR ${EVE_SCENE_PROBE_ASSETS_DIR}/dx9/model/spaceobjectfactory/factions)
set(EVE_SCENE_PROBE_CLIENT_MATERIAL_DIR ${EVE_SCENE_PROBE_ASSETS_DIR}/dx9/model/spaceobjectfactory/materials)
set(EVE_SCENE_PROBE_CLIENT_EFFECT_DIR ${EVE_SCENE_PROBE_ASSETS_DIR}/graphics/effect.metal/managed/space/spaceobject/v5/quad)
set(EVE_SCENE_PROBE_CLIENT_FX_EFFECT_DIR ${EVE_SCENE_PROBE_ASSETS_DIR}/graphics/effect.metal/managed/space/spaceobject/v5/fx)
set(EVE_SCENE_PROBE_CLIENT_SPRITE_TEXTURE_DIR ${EVE_SCENE_PROBE_ASSETS_DIR}/texture/sprite)
set(EVE_SCENE_PROBE_CLIENT_FITTING_DIR ${EVE_SCENE_PROBE_ASSETS_DIR}/dx9/scene/fitting)
set(EVE_SCENE_PROBE_ASTERO_HULL_BLACK ${EVE_SCENE_PROBE_CLIENT_HULL_DIR}/soef1_t1.black)
set(EVE_SCENE_PROBE_ASTERO_FACTION_BLACK ${EVE_SCENE_PROBE_CLIENT_FACTION_DIR}/soebase.black)
set(EVE_SCENE_PROBE_CHROME_MATERIAL_BLACK ${EVE_SCENE_PROBE_CLIENT_MATERIAL_DIR}/chrome_metallic.black)
set(EVE_SCENE_PROBE_STEEL_MATERIAL_BLACK ${EVE_SCENE_PROBE_CLIENT_MATERIAL_DIR}/grey_steel_brushed.black)
set(EVE_SCENE_PROBE_WHITE_MATERIAL_BLACK ${EVE_SCENE_PROBE_CLIENT_MATERIAL_DIR}/white_ghost_matt.black)
set(EVE_SCENE_PROBE_RED_MATERIAL_BLACK ${EVE_SCENE_PROBE_CLIENT_MATERIAL_DIR}/red_crimson_enamel.black)
set(EVE_SCENE_PROBE_GLASS_MATERIAL_BLACK ${EVE_SCENE_PROBE_CLIENT_MATERIAL_DIR}/glass_soe.black)
set(EVE_SCENE_PROBE_QUADV5_HI ${EVE_SCENE_PROBE_CLIENT_EFFECT_DIR}/quadv5.sm_hi)
set(EVE_SCENE_PROBE_QUADV5_LO ${EVE_SCENE_PROBE_CLIENT_EFFECT_DIR}/quadv5.sm_lo)
set(EVE_SCENE_PROBE_QUADV5_DEPTH ${EVE_SCENE_PROBE_CLIENT_EFFECT_DIR}/quadv5.sm_depth)
set(EVE_SCENE_PROBE_QUADHEATV5_HI ${EVE_SCENE_PROBE_CLIENT_EFFECT_DIR}/quadheatv5.sm_hi)
set(EVE_SCENE_PROBE_QUADHEATV5_LO ${EVE_SCENE_PROBE_CLIENT_EFFECT_DIR}/quadheatv5.sm_lo)
set(EVE_SCENE_PROBE_QUADHEATV5_DEPTH ${EVE_SCENE_PROBE_CLIENT_EFFECT_DIR}/quadheatv5.sm_depth)
set(EVE_SCENE_PROBE_FXDISTORTIONV5_HI ${EVE_SCENE_PROBE_CLIENT_FX_EFFECT_DIR}/fxdistortionv5.sm_hi)
set(EVE_SCENE_PROBE_FXDISTORTIONV5_LO ${EVE_SCENE_PROBE_CLIENT_FX_EFFECT_DIR}/fxdistortionv5.sm_lo)
set(EVE_SCENE_PROBE_FXDISTORTIONV5_DEPTH ${EVE_SCENE_PROBE_CLIENT_FX_EFFECT_DIR}/fxdistortionv5.sm_depth)
set(EVE_SCENE_PROBE_WARP_DISTORTION ${EVE_SCENE_PROBE_CLIENT_SPRITE_TEXTURE_DIR}/warptunnel4_n.dds)
set(EVE_SCENE_PROBE_FITTING_SCENE ${EVE_SCENE_PROBE_CLIENT_FITTING_DIR}/fitting.black)
set(EVE_SCENE_PROBE_FITTING_CUBE ${EVE_SCENE_PROBE_CLIENT_FITTING_DIR}/fitting_cube.dds)
set(EVE_SCENE_PROBE_FITTING_CUBE_BLUR ${EVE_SCENE_PROBE_CLIENT_FITTING_DIR}/fitting_cube_blur.dds)
set(EVE_SCENE_PROBE_FITTING_CUBE_REFL ${EVE_SCENE_PROBE_CLIENT_FITTING_DIR}/fitting_cube_refl.dds)
set(EVE_SCENE_PROBE_DEFAULT_POSTPROCESS ${EVE_SCENE_PROBE_ASSETS_DIR}/dx9/default/postprocess.black)
set(EVE_SCENE_PROBE_A01_DIR ${EVE_SCENE_PROBE_ASSETS_DIR}/dx9/scene/universe)
set(EVE_SCENE_PROBE_A01_LOW_QUALITY_DIR ${EVE_SCENE_PROBE_A01_DIR}/lowquality)
set(EVE_SCENE_PROBE_A01_SCENE ${EVE_SCENE_PROBE_A01_DIR}/a01_cube.black)
set(EVE_SCENE_PROBE_A01_CUBE ${EVE_SCENE_PROBE_A01_DIR}/a01_cube.dds)
set(EVE_SCENE_PROBE_A01_REFLECTION ${EVE_SCENE_PROBE_A01_DIR}/a01_cube_refl.dds)
set(EVE_SCENE_PROBE_A01_BLUR ${EVE_SCENE_PROBE_A01_DIR}/a01_cube_blur.dds)
set(EVE_SCENE_PROBE_A01_LOW_DETAIL ${EVE_SCENE_PROBE_A01_DIR}/a01_cube_lowdetail.dds)
set(EVE_SCENE_PROBE_A01_LOW_QUALITY ${EVE_SCENE_PROBE_A01_LOW_QUALITY_DIR}/wormhole_a01.dds)
set(EVE_SCENE_PROBE_A01_LOW_QUALITY_MIX ${EVE_SCENE_PROBE_A01_LOW_QUALITY_DIR}/wormhole_a01_mix.dds)
set(EVE_SCENE_PROBE_A01_OVERLAY ${EVE_SCENE_PROBE_ASSETS_DIR}/texture/fx/misc/rgb_01a.dds)
set(EVE_SCENE_PROBE_A01_STARS ${EVE_SCENE_PROBE_ASSETS_DIR}/dx9/scene/starfield/stars01_tile2.dds)
set(EVE_SCENE_PROBE_BACKGROUND_EFFECT_DIR ${EVE_SCENE_PROBE_ASSETS_DIR}/graphics/effect.metal/managed/space/specialfx)
set(EVE_SCENE_PROBE_BACKGROUND_HI ${EVE_SCENE_PROBE_BACKGROUND_EFFECT_DIR}/background.sm_hi)
set(EVE_SCENE_PROBE_BACKGROUND_LO ${EVE_SCENE_PROBE_BACKGROUND_EFFECT_DIR}/background.sm_lo)
set(EVE_SCENE_PROBE_BACKGROUND_DEPTH ${EVE_SCENE_PROBE_BACKGROUND_EFFECT_DIR}/background.sm_depth)
set(EVE_SCENE_PROBE_A01_MANIFEST ${CMAKE_CURRENT_BINARY_DIR}/Reports/A01SceneResources.json)
set(EVE_SCENE_PROBE_POSTPROCESS_MANIFEST ${CMAKE_CURRENT_BINARY_DIR}/Reports/PostProcessResources.json)
set(EVE_SCENE_PROBE_CLIENT_MANIFEST ${CMAKE_CURRENT_BINARY_DIR}/Reports/AsteroClientResources.json)
set(EVE_SCENE_PROBE_ASTERO_MODEL_MANIFEST ${CMAKE_CURRENT_BINARY_DIR}/Reports/AsteroModelResources.json)
set(EVE_SCENE_PROBE_SPRITE_STARS ${EVE_SCENE_PROBE_ASSETS_DIR}/dx9/scene/starfield/spritestars.black)
set(EVE_SCENE_PROBE_STAR_ATLAS ${EVE_SCENE_PROBE_ASSETS_DIR}/dx9/scene/starfield/staratlas.dds)
set(EVE_SCENE_PROBE_STAR_COLORS ${EVE_SCENE_PROBE_ASSETS_DIR}/dx9/scene/starfield/starcolors.dds)
set(EVE_SCENE_PROBE_STAR_SPRITES_HI ${EVE_SCENE_PROBE_BACKGROUND_EFFECT_DIR}/starsprites.sm_hi)
set(EVE_SCENE_PROBE_STAR_SPRITES_LO ${EVE_SCENE_PROBE_BACKGROUND_EFFECT_DIR}/starsprites.sm_lo)
set(EVE_SCENE_PROBE_STAR_SPRITES_DEPTH ${EVE_SCENE_PROBE_BACKGROUND_EFFECT_DIR}/starsprites.sm_depth)
set(EVE_SCENE_PROBE_NEW_EDEN_SUN ${EVE_SCENE_PROBE_ASSETS_DIR}/dx9/model/celestial/sun/sun_yellow_small_01b.black)
set(EVE_SCENE_PROBE_NEW_EDEN_PLANET ${EVE_SCENE_PROBE_ASSETS_DIR}/dx9/model/worldobject/planet/sandstormplanet.black)
set(EVE_SCENE_PROBE_NEW_EDEN_MANIFEST ${CMAKE_CURRENT_BINARY_DIR}/Reports/NewEdenSceneResources.json)
set(EVE_SCENE_PROBE_COMPUTE_LIGHT_LISTS_HI ${EVE_SCENE_PROBE_SYSTEM_EFFECT_DIR}/computelightlists.sm_hi)
set(EVE_SCENE_PROBE_COMPUTE_LIGHT_LISTS_LO ${EVE_SCENE_PROBE_SYSTEM_EFFECT_DIR}/computelightlists.sm_lo)
set(EVE_SCENE_PROBE_COMPUTE_LIGHT_LISTS_DEPTH ${EVE_SCENE_PROBE_SYSTEM_EFFECT_DIR}/computelightlists.sm_depth)
set(EVE_SCENE_PROBE_LOCAL_LIGHT_MANIFEST ${CMAKE_CURRENT_BINARY_DIR}/Reports/AsteroLocalLightResources.json)
set(EVE_SCENE_PROBE_LOCAL_LIGHT_BANNER_DIR ${CMAKE_CURRENT_BINARY_DIR}/LocalLightAssets/texture/global)
set(EVE_SCENE_PROBE_LOCAL_LIGHT_BANNER_BLACK ${EVE_SCENE_PROBE_LOCAL_LIGHT_BANNER_DIR}/black.dds)
set(EVE_SCENE_PROBE_LOCAL_LIGHT_SOE_BANNER_DIR ${CMAKE_CURRENT_BINARY_DIR}/LocalLightAssets/ui/texture/classes/banners/factional/various)
set(EVE_SCENE_PROBE_LOCAL_LIGHT_SOE_BANNER ${EVE_SCENE_PROBE_LOCAL_LIGHT_SOE_BANNER_DIR}/soe_banner_base.dds)
set(EVE_SCENE_PROBE_VCPKG_LIB_DIR ${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/lib)

function(add_eve_scene_probe_dds_conversion input output metadata)
    add_custom_command(
        OUTPUT ${output} ${metadata}
        COMMAND $<TARGET_FILE:TrinityDdsToRgba>
            --input ${input}
            --output ${output}
            --metadata ${metadata}
            --summary
        DEPENDS TrinityDdsToRgba ${input}
        VERBATIM
    )
endfunction()

add_custom_target(TrinityEveSceneProbeAssets
    COMMAND ${CMAKE_COMMAND} -E make_directory ${EVE_SCENE_PROBE_ASSETS_DIR}
    VERBATIM
)

add_custom_command(
	OUTPUT
		${EVE_SCENE_PROBE_EFFECT}
		${EVE_SCENE_PROBE_EFFECT_DEPTH}
		${EVE_SCENE_PROBE_RENDER_PRODUCT_EFFECT}
		${EVE_SCENE_PROBE_RENDER_PRODUCT_EFFECT_DEPTH}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${EVE_SCENE_PROBE_EFFECT_DIR}
	COMMAND $<TARGET_FILE:ShaderCompiler>
		/single /O3
        /define SHADERMODEL 4
        /define PLATFORM 10
        ${EVE_SCENE_PROBE_EFFECT_SOURCE}
        ${EVE_SCENE_PROBE_EFFECT}
    COMMAND $<TARGET_FILE:ShaderCompiler>
        /single /O3
        /define SHADERMODEL 5
        /define PLATFORM 10
		${EVE_SCENE_PROBE_EFFECT_SOURCE}
		${EVE_SCENE_PROBE_EFFECT_DEPTH}
	COMMAND $<TARGET_FILE:ShaderCompiler>
		/single /O3
		/define SHADERMODEL 4
		/define PLATFORM 10
		${EVE_SCENE_PROBE_RENDER_PRODUCT_EFFECT_SOURCE}
		${EVE_SCENE_PROBE_RENDER_PRODUCT_EFFECT}
	COMMAND $<TARGET_FILE:ShaderCompiler>
		/single /O3
		/define SHADERMODEL 5
		/define PLATFORM 10
		${EVE_SCENE_PROBE_RENDER_PRODUCT_EFFECT_SOURCE}
		${EVE_SCENE_PROBE_RENDER_PRODUCT_EFFECT_DEPTH}
	DEPENDS ShaderCompiler ${EVE_SCENE_PROBE_EFFECT_SOURCE} ${EVE_SCENE_PROBE_RENDER_PRODUCT_EFFECT_SOURCE}
	VERBATIM
)
add_custom_target(TrinityEveSceneProbeEffect DEPENDS
	${EVE_SCENE_PROBE_EFFECT}
	${EVE_SCENE_PROBE_EFFECT_DEPTH}
	${EVE_SCENE_PROBE_RENDER_PRODUCT_EFFECT}
	${EVE_SCENE_PROBE_RENDER_PRODUCT_EFFECT_DEPTH}
)
add_dependencies(TrinityEveSceneProbeAssets TrinityEveSceneProbeEffect)

if(EXISTS "${EVE_SCENE_PROBE_SHARED_CACHE_MACOS_INDEX}")
    add_custom_command(
        OUTPUT
            ${EVE_SCENE_PROBE_COMPUTE_LIGHT_LISTS_HI}
            ${EVE_SCENE_PROBE_COMPUTE_LIGHT_LISTS_LO}
            ${EVE_SCENE_PROBE_COMPUTE_LIGHT_LISTS_DEPTH}
            ${EVE_SCENE_PROBE_LOCAL_LIGHT_BANNER_BLACK}
            ${EVE_SCENE_PROBE_LOCAL_LIGHT_SOE_BANNER}
            ${EVE_SCENE_PROBE_LOCAL_LIGHT_MANIFEST}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${EVE_SCENE_PROBE_SYSTEM_EFFECT_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${EVE_SCENE_PROBE_LOCAL_LIGHT_BANNER_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${EVE_SCENE_PROBE_LOCAL_LIGHT_SOE_BANNER_DIR}
        COMMAND ${TRINITY_SHARED_CACHE_NODE_TOOL} ${TRINITY_SHARED_CACHE_TO_GLTF_SCRIPT}
            --cache-root ${EVE_SCENE_PROBE_SHARED_CACHE}
            --copy-resource ${EVE_SCENE_PROBE_COMPUTE_LIGHT_LISTS_HI_RESOURCE} ${EVE_SCENE_PROBE_COMPUTE_LIGHT_LISTS_HI}
            --copy-resource ${EVE_SCENE_PROBE_COMPUTE_LIGHT_LISTS_LO_RESOURCE} ${EVE_SCENE_PROBE_COMPUTE_LIGHT_LISTS_LO}
            --copy-resource ${EVE_SCENE_PROBE_COMPUTE_LIGHT_LISTS_DEPTH_RESOURCE} ${EVE_SCENE_PROBE_COMPUTE_LIGHT_LISTS_DEPTH}
            --copy-resource res:/texture/global/black.dds ${EVE_SCENE_PROBE_LOCAL_LIGHT_BANNER_BLACK}
            --copy-resource res:/ui/texture/classes/banners/factional/various/soe_banner_base.dds ${EVE_SCENE_PROBE_LOCAL_LIGHT_SOE_BANNER}
            --manifest ${EVE_SCENE_PROBE_LOCAL_LIGHT_MANIFEST}
            --summary
        DEPENDS TrinitySharedCacheToGltf ${EVE_SCENE_PROBE_SHARED_CACHE_MACOS_INDEX}
        VERBATIM
    )
    add_custom_target(TrinityEveSceneProbeLocalLightAssets
        DEPENDS
            ${EVE_SCENE_PROBE_COMPUTE_LIGHT_LISTS_HI}
            ${EVE_SCENE_PROBE_COMPUTE_LIGHT_LISTS_LO}
            ${EVE_SCENE_PROBE_COMPUTE_LIGHT_LISTS_DEPTH}
            ${EVE_SCENE_PROBE_LOCAL_LIGHT_BANNER_BLACK}
            ${EVE_SCENE_PROBE_LOCAL_LIGHT_SOE_BANNER}
            ${EVE_SCENE_PROBE_LOCAL_LIGHT_MANIFEST}
    )
    add_dependencies(TrinityEveSceneProbeAssets TrinityEveSceneProbeLocalLightAssets)

    add_custom_command(
        OUTPUT
            ${EVE_SCENE_PROBE_TONEMAPPING_EFFECT}
            ${EVE_SCENE_PROBE_TONEMAPPING_EFFECT_DEPTH}
            ${EVE_SCENE_PROBE_CREATE_HISTOGRAMS_EFFECT}
            ${EVE_SCENE_PROBE_CREATE_HISTOGRAMS_EFFECT_DEPTH}
            ${EVE_SCENE_PROBE_MERGE_HISTOGRAMS_EFFECT}
            ${EVE_SCENE_PROBE_MERGE_HISTOGRAMS_EFFECT_DEPTH}
            ${EVE_SCENE_PROBE_MEASURE_EXPOSURE_EFFECT}
            ${EVE_SCENE_PROBE_MEASURE_EXPOSURE_EFFECT_DEPTH}
            ${EVE_SCENE_PROBE_BLIT_EFFECT}
            ${EVE_SCENE_PROBE_BLIT_EFFECT_DEPTH}
            ${EVE_SCENE_PROBE_BLIT_FILTERED_EFFECT}
            ${EVE_SCENE_PROBE_BLIT_FILTERED_EFFECT_DEPTH}
            ${EVE_SCENE_PROBE_BLACK_TEXTURE}
            ${EVE_SCENE_PROBE_REFLECTION_TEXTURE}
            ${EVE_SCENE_PROBE_FLAT_NORMAL_TEXTURE}
            ${EVE_SCENE_PROBE_REFLECTION_CORRECTION_TEXTURE}
            ${EVE_SCENE_PROBE_POSTPROCESS_MANIFEST}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${EVE_SCENE_PROBE_POSTPROCESS_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${EVE_SCENE_PROBE_SYSTEM_EFFECT_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${EVE_SCENE_PROBE_GLOBAL_TEXTURE_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${EVE_SCENE_PROBE_ASSETS_DIR}/texture/reflectioncorrection
        COMMAND ${TRINITY_SHARED_CACHE_NODE_TOOL} ${TRINITY_SHARED_CACHE_TO_GLTF_SCRIPT}
            --cache-root ${EVE_SCENE_PROBE_SHARED_CACHE}
            --copy-resource res:/graphics/effect.metal/managed/space/postprocess/tonemapping.sm_hi ${EVE_SCENE_PROBE_TONEMAPPING_EFFECT}
            --copy-resource res:/graphics/effect.metal/managed/space/postprocess/tonemapping.sm_depth ${EVE_SCENE_PROBE_TONEMAPPING_EFFECT_DEPTH}
            --copy-resource res:/graphics/effect.metal/managed/space/postprocess/createhistograms.sm_hi ${EVE_SCENE_PROBE_CREATE_HISTOGRAMS_EFFECT}
            --copy-resource res:/graphics/effect.metal/managed/space/postprocess/createhistograms.sm_depth ${EVE_SCENE_PROBE_CREATE_HISTOGRAMS_EFFECT_DEPTH}
            --copy-resource res:/graphics/effect.metal/managed/space/postprocess/mergehistograms.sm_hi ${EVE_SCENE_PROBE_MERGE_HISTOGRAMS_EFFECT}
            --copy-resource res:/graphics/effect.metal/managed/space/postprocess/mergehistograms.sm_depth ${EVE_SCENE_PROBE_MERGE_HISTOGRAMS_EFFECT_DEPTH}
            --copy-resource res:/graphics/effect.metal/managed/space/postprocess/measureexposure.sm_hi ${EVE_SCENE_PROBE_MEASURE_EXPOSURE_EFFECT}
            --copy-resource res:/graphics/effect.metal/managed/space/postprocess/measureexposure.sm_depth ${EVE_SCENE_PROBE_MEASURE_EXPOSURE_EFFECT_DEPTH}
            --copy-resource res:/graphics/effect.metal/managed/space/system/blit.sm_hi ${EVE_SCENE_PROBE_BLIT_EFFECT}
            --copy-resource res:/graphics/effect.metal/managed/space/system/blit.sm_depth ${EVE_SCENE_PROBE_BLIT_EFFECT_DEPTH}
            --copy-resource res:/graphics/effect.metal/managed/space/system/blitfiltered.sm_hi ${EVE_SCENE_PROBE_BLIT_FILTERED_EFFECT}
            --copy-resource res:/graphics/effect.metal/managed/space/system/blitfiltered.sm_depth ${EVE_SCENE_PROBE_BLIT_FILTERED_EFFECT_DEPTH}
            --copy-resource res:/texture/global/black.dds ${EVE_SCENE_PROBE_BLACK_TEXTURE}
            --copy-resource res:/texture/global/genericreflection_cube.dds ${EVE_SCENE_PROBE_REFLECTION_TEXTURE}
            --copy-resource ${EVE_SCENE_PROBE_FLAT_NORMAL_RESOURCE} ${EVE_SCENE_PROBE_FLAT_NORMAL_TEXTURE}
            --copy-resource ${EVE_SCENE_PROBE_REFLECTION_CORRECTION_RESOURCE} ${EVE_SCENE_PROBE_REFLECTION_CORRECTION_TEXTURE}
            --manifest ${EVE_SCENE_PROBE_POSTPROCESS_MANIFEST}
            --summary
        DEPENDS
            TrinitySharedCacheToGltf
            ${EVE_SCENE_PROBE_SHARED_CACHE_MACOS_INDEX}
        VERBATIM
    )
    add_custom_target(TrinityEveSceneProbePostProcessEffects
        DEPENDS
            ${EVE_SCENE_PROBE_TONEMAPPING_EFFECT}
            ${EVE_SCENE_PROBE_TONEMAPPING_EFFECT_DEPTH}
            ${EVE_SCENE_PROBE_CREATE_HISTOGRAMS_EFFECT}
            ${EVE_SCENE_PROBE_CREATE_HISTOGRAMS_EFFECT_DEPTH}
            ${EVE_SCENE_PROBE_MERGE_HISTOGRAMS_EFFECT}
            ${EVE_SCENE_PROBE_MERGE_HISTOGRAMS_EFFECT_DEPTH}
            ${EVE_SCENE_PROBE_MEASURE_EXPOSURE_EFFECT}
            ${EVE_SCENE_PROBE_MEASURE_EXPOSURE_EFFECT_DEPTH}
            ${EVE_SCENE_PROBE_BLIT_EFFECT}
            ${EVE_SCENE_PROBE_BLIT_EFFECT_DEPTH}
            ${EVE_SCENE_PROBE_BLIT_FILTERED_EFFECT}
            ${EVE_SCENE_PROBE_BLIT_FILTERED_EFFECT_DEPTH}
            ${EVE_SCENE_PROBE_BLACK_TEXTURE}
            ${EVE_SCENE_PROBE_REFLECTION_TEXTURE}
            ${EVE_SCENE_PROBE_FLAT_NORMAL_TEXTURE}
            ${EVE_SCENE_PROBE_REFLECTION_CORRECTION_TEXTURE}
            ${EVE_SCENE_PROBE_POSTPROCESS_MANIFEST}
    )
    add_dependencies(TrinityEveSceneProbeAssets TrinityEveSceneProbePostProcessEffects)
endif()

if(EXISTS "${EVE_SCENE_PROBE_SHARED_CACHE_INDEX}" AND EXISTS "${EVE_SCENE_PROBE_SHARED_CACHE_MACOS_INDEX}")
    add_custom_command(
        OUTPUT
            ${EVE_SCENE_PROBE_ASTERO_HULL_BLACK}
            ${EVE_SCENE_PROBE_ASTERO_FACTION_BLACK}
            ${EVE_SCENE_PROBE_CHROME_MATERIAL_BLACK}
            ${EVE_SCENE_PROBE_STEEL_MATERIAL_BLACK}
            ${EVE_SCENE_PROBE_WHITE_MATERIAL_BLACK}
            ${EVE_SCENE_PROBE_RED_MATERIAL_BLACK}
            ${EVE_SCENE_PROBE_GLASS_MATERIAL_BLACK}
            ${EVE_SCENE_PROBE_QUADV5_HI}
            ${EVE_SCENE_PROBE_QUADV5_LO}
            ${EVE_SCENE_PROBE_QUADV5_DEPTH}
            ${EVE_SCENE_PROBE_QUADHEATV5_HI}
            ${EVE_SCENE_PROBE_QUADHEATV5_LO}
            ${EVE_SCENE_PROBE_QUADHEATV5_DEPTH}
            ${EVE_SCENE_PROBE_FXDISTORTIONV5_HI}
            ${EVE_SCENE_PROBE_FXDISTORTIONV5_LO}
            ${EVE_SCENE_PROBE_FXDISTORTIONV5_DEPTH}
            ${EVE_SCENE_PROBE_WARP_DISTORTION}
            ${EVE_SCENE_PROBE_FITTING_SCENE}
            ${EVE_SCENE_PROBE_FITTING_CUBE}
            ${EVE_SCENE_PROBE_FITTING_CUBE_BLUR}
            ${EVE_SCENE_PROBE_FITTING_CUBE_REFL}
            ${EVE_SCENE_PROBE_DEFAULT_POSTPROCESS}
            ${EVE_SCENE_PROBE_CLIENT_MANIFEST}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${EVE_SCENE_PROBE_CLIENT_HULL_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${EVE_SCENE_PROBE_CLIENT_FACTION_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${EVE_SCENE_PROBE_CLIENT_MATERIAL_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${EVE_SCENE_PROBE_CLIENT_EFFECT_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${EVE_SCENE_PROBE_CLIENT_FX_EFFECT_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${EVE_SCENE_PROBE_CLIENT_SPRITE_TEXTURE_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${EVE_SCENE_PROBE_CLIENT_FITTING_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${EVE_SCENE_PROBE_ASSETS_DIR}/dx9/default
        COMMAND ${TRINITY_SHARED_CACHE_NODE_TOOL} ${TRINITY_SHARED_CACHE_TO_GLTF_SCRIPT}
            --cache-root ${EVE_SCENE_PROBE_SHARED_CACHE}
            --copy-resource ${EVE_SCENE_PROBE_ASTERO_HULL_BLACK_RESOURCE} ${EVE_SCENE_PROBE_ASTERO_HULL_BLACK}
            --copy-resource ${EVE_SCENE_PROBE_ASTERO_FACTION_BLACK_RESOURCE} ${EVE_SCENE_PROBE_ASTERO_FACTION_BLACK}
            --copy-resource res:/dx9/model/spaceobjectfactory/materials/chrome_metallic.black ${EVE_SCENE_PROBE_CHROME_MATERIAL_BLACK}
            --copy-resource res:/dx9/model/spaceobjectfactory/materials/grey_steel_brushed.black ${EVE_SCENE_PROBE_STEEL_MATERIAL_BLACK}
            --copy-resource res:/dx9/model/spaceobjectfactory/materials/white_ghost_matt.black ${EVE_SCENE_PROBE_WHITE_MATERIAL_BLACK}
            --copy-resource res:/dx9/model/spaceobjectfactory/materials/red_crimson_enamel.black ${EVE_SCENE_PROBE_RED_MATERIAL_BLACK}
            --copy-resource res:/dx9/model/spaceobjectfactory/materials/glass_soe.black ${EVE_SCENE_PROBE_GLASS_MATERIAL_BLACK}
            --copy-resource ${EVE_SCENE_PROBE_QUADV5_HI_RESOURCE} ${EVE_SCENE_PROBE_QUADV5_HI}
            --copy-resource ${EVE_SCENE_PROBE_QUADV5_LO_RESOURCE} ${EVE_SCENE_PROBE_QUADV5_LO}
            --copy-resource ${EVE_SCENE_PROBE_QUADV5_DEPTH_RESOURCE} ${EVE_SCENE_PROBE_QUADV5_DEPTH}
            --copy-resource ${EVE_SCENE_PROBE_QUADHEATV5_HI_RESOURCE} ${EVE_SCENE_PROBE_QUADHEATV5_HI}
            --copy-resource ${EVE_SCENE_PROBE_QUADHEATV5_LO_RESOURCE} ${EVE_SCENE_PROBE_QUADHEATV5_LO}
            --copy-resource ${EVE_SCENE_PROBE_QUADHEATV5_DEPTH_RESOURCE} ${EVE_SCENE_PROBE_QUADHEATV5_DEPTH}
            --copy-resource ${EVE_SCENE_PROBE_FXDISTORTIONV5_HI_RESOURCE} ${EVE_SCENE_PROBE_FXDISTORTIONV5_HI}
            --copy-resource ${EVE_SCENE_PROBE_FXDISTORTIONV5_LO_RESOURCE} ${EVE_SCENE_PROBE_FXDISTORTIONV5_LO}
            --copy-resource ${EVE_SCENE_PROBE_FXDISTORTIONV5_DEPTH_RESOURCE} ${EVE_SCENE_PROBE_FXDISTORTIONV5_DEPTH}
            --copy-resource ${EVE_SCENE_PROBE_WARP_DISTORTION_RESOURCE} ${EVE_SCENE_PROBE_WARP_DISTORTION}
            --copy-resource ${EVE_SCENE_PROBE_FITTING_SCENE_RESOURCE} ${EVE_SCENE_PROBE_FITTING_SCENE}
            --copy-resource ${EVE_SCENE_PROBE_FITTING_CUBE_RESOURCE} ${EVE_SCENE_PROBE_FITTING_CUBE}
            --copy-resource ${EVE_SCENE_PROBE_FITTING_CUBE_BLUR_RESOURCE} ${EVE_SCENE_PROBE_FITTING_CUBE_BLUR}
            --copy-resource ${EVE_SCENE_PROBE_FITTING_CUBE_REFL_RESOURCE} ${EVE_SCENE_PROBE_FITTING_CUBE_REFL}
            --copy-resource ${EVE_SCENE_PROBE_DEFAULT_POSTPROCESS_RESOURCE} ${EVE_SCENE_PROBE_DEFAULT_POSTPROCESS}
            --manifest ${EVE_SCENE_PROBE_CLIENT_MANIFEST}
            --summary
        DEPENDS
            TrinitySharedCacheToGltf
            ${TRINITY_SHARED_CACHE_TO_GLTF_SCRIPT}
            ${EVE_SCENE_PROBE_SHARED_CACHE_INDEX}
            ${EVE_SCENE_PROBE_SHARED_CACHE_MACOS_INDEX}
        VERBATIM
    )
    add_custom_target(TrinityEveSceneProbeClientAssets
        DEPENDS
            ${EVE_SCENE_PROBE_ASTERO_HULL_BLACK}
            ${EVE_SCENE_PROBE_ASTERO_FACTION_BLACK}
            ${EVE_SCENE_PROBE_CHROME_MATERIAL_BLACK}
            ${EVE_SCENE_PROBE_STEEL_MATERIAL_BLACK}
            ${EVE_SCENE_PROBE_WHITE_MATERIAL_BLACK}
            ${EVE_SCENE_PROBE_RED_MATERIAL_BLACK}
            ${EVE_SCENE_PROBE_GLASS_MATERIAL_BLACK}
            ${EVE_SCENE_PROBE_QUADV5_HI}
            ${EVE_SCENE_PROBE_QUADV5_LO}
            ${EVE_SCENE_PROBE_QUADV5_DEPTH}
            ${EVE_SCENE_PROBE_QUADHEATV5_HI}
            ${EVE_SCENE_PROBE_QUADHEATV5_LO}
            ${EVE_SCENE_PROBE_QUADHEATV5_DEPTH}
            ${EVE_SCENE_PROBE_FXDISTORTIONV5_HI}
            ${EVE_SCENE_PROBE_FXDISTORTIONV5_LO}
            ${EVE_SCENE_PROBE_FXDISTORTIONV5_DEPTH}
            ${EVE_SCENE_PROBE_WARP_DISTORTION}
            ${EVE_SCENE_PROBE_FITTING_SCENE}
            ${EVE_SCENE_PROBE_FITTING_CUBE}
            ${EVE_SCENE_PROBE_FITTING_CUBE_BLUR}
            ${EVE_SCENE_PROBE_FITTING_CUBE_REFL}
            ${EVE_SCENE_PROBE_DEFAULT_POSTPROCESS}
            ${EVE_SCENE_PROBE_CLIENT_MANIFEST}
    )
    add_dependencies(TrinityEveSceneProbeAssets TrinityEveSceneProbeClientAssets)
endif()

if(EXISTS "${EVE_SCENE_PROBE_SHARED_CACHE_INDEX}" AND EXISTS "${EVE_SCENE_PROBE_SHARED_CACHE_MACOS_INDEX}")
    add_custom_command(
        OUTPUT
            ${EVE_SCENE_PROBE_A01_SCENE}
            ${EVE_SCENE_PROBE_A01_CUBE}
            ${EVE_SCENE_PROBE_A01_REFLECTION}
            ${EVE_SCENE_PROBE_A01_BLUR}
            ${EVE_SCENE_PROBE_A01_LOW_DETAIL}
            ${EVE_SCENE_PROBE_A01_LOW_QUALITY}
            ${EVE_SCENE_PROBE_A01_LOW_QUALITY_MIX}
            ${EVE_SCENE_PROBE_A01_OVERLAY}
            ${EVE_SCENE_PROBE_A01_STARS}
            ${EVE_SCENE_PROBE_BACKGROUND_HI}
            ${EVE_SCENE_PROBE_BACKGROUND_LO}
            ${EVE_SCENE_PROBE_BACKGROUND_DEPTH}
            ${EVE_SCENE_PROBE_A01_MANIFEST}
        COMMAND ${TRINITY_SHARED_CACHE_NODE_TOOL} ${TRINITY_SHARED_CACHE_TO_GLTF_SCRIPT}
            --cache-root ${EVE_SCENE_PROBE_SHARED_CACHE}
            --copy-resource ${EVE_SCENE_PROBE_A01_SCENE_RESOURCE} ${EVE_SCENE_PROBE_A01_SCENE}
            --copy-resource ${EVE_SCENE_PROBE_A01_CUBE_RESOURCE} ${EVE_SCENE_PROBE_A01_CUBE}
            --copy-resource ${EVE_SCENE_PROBE_A01_REFLECTION_RESOURCE} ${EVE_SCENE_PROBE_A01_REFLECTION}
            --copy-resource ${EVE_SCENE_PROBE_A01_BLUR_RESOURCE} ${EVE_SCENE_PROBE_A01_BLUR}
            --copy-resource ${EVE_SCENE_PROBE_A01_LOW_DETAIL_RESOURCE} ${EVE_SCENE_PROBE_A01_LOW_DETAIL}
            --copy-resource ${EVE_SCENE_PROBE_A01_LOW_QUALITY_RESOURCE} ${EVE_SCENE_PROBE_A01_LOW_QUALITY}
            --copy-resource ${EVE_SCENE_PROBE_A01_LOW_QUALITY_MIX_RESOURCE} ${EVE_SCENE_PROBE_A01_LOW_QUALITY_MIX}
            --copy-resource ${EVE_SCENE_PROBE_A01_OVERLAY_RESOURCE} ${EVE_SCENE_PROBE_A01_OVERLAY}
            --copy-resource ${EVE_SCENE_PROBE_A01_STARS_RESOURCE} ${EVE_SCENE_PROBE_A01_STARS}
            --copy-resource ${EVE_SCENE_PROBE_BACKGROUND_HI_RESOURCE} ${EVE_SCENE_PROBE_BACKGROUND_HI}
            --copy-resource ${EVE_SCENE_PROBE_BACKGROUND_LO_RESOURCE} ${EVE_SCENE_PROBE_BACKGROUND_LO}
            --copy-resource ${EVE_SCENE_PROBE_BACKGROUND_DEPTH_RESOURCE} ${EVE_SCENE_PROBE_BACKGROUND_DEPTH}
            --manifest ${EVE_SCENE_PROBE_A01_MANIFEST}
            --summary
        DEPENDS
            TrinitySharedCacheToGltf
            ${EVE_SCENE_PROBE_SHARED_CACHE_INDEX}
            ${EVE_SCENE_PROBE_SHARED_CACHE_MACOS_INDEX}
        VERBATIM
    )
    add_custom_target(TrinityEveSceneProbeA01Assets
        DEPENDS
            ${EVE_SCENE_PROBE_A01_SCENE}
            ${EVE_SCENE_PROBE_A01_CUBE}
            ${EVE_SCENE_PROBE_A01_REFLECTION}
            ${EVE_SCENE_PROBE_A01_BLUR}
            ${EVE_SCENE_PROBE_A01_LOW_DETAIL}
            ${EVE_SCENE_PROBE_A01_LOW_QUALITY}
            ${EVE_SCENE_PROBE_A01_LOW_QUALITY_MIX}
            ${EVE_SCENE_PROBE_A01_OVERLAY}
            ${EVE_SCENE_PROBE_A01_STARS}
            ${EVE_SCENE_PROBE_BACKGROUND_HI}
            ${EVE_SCENE_PROBE_BACKGROUND_LO}
            ${EVE_SCENE_PROBE_BACKGROUND_DEPTH}
            ${EVE_SCENE_PROBE_A01_MANIFEST}
    )
    add_dependencies(TrinityEveSceneProbeAssets TrinityEveSceneProbeA01Assets)
endif()

if(TARGET TrinityEveSceneProbeA01Assets)
    add_custom_command(
        OUTPUT
            ${EVE_SCENE_PROBE_SPRITE_STARS}
            ${EVE_SCENE_PROBE_STAR_ATLAS}
            ${EVE_SCENE_PROBE_STAR_COLORS}
            ${EVE_SCENE_PROBE_STAR_SPRITES_HI}
            ${EVE_SCENE_PROBE_STAR_SPRITES_LO}
            ${EVE_SCENE_PROBE_STAR_SPRITES_DEPTH}
            ${EVE_SCENE_PROBE_NEW_EDEN_SUN}
            ${EVE_SCENE_PROBE_NEW_EDEN_PLANET}
            ${EVE_SCENE_PROBE_NEW_EDEN_MANIFEST}
        COMMAND ${TRINITY_SHARED_CACHE_NODE_TOOL} ${TRINITY_SHARED_CACHE_TO_GLTF_SCRIPT}
            --cache-root ${EVE_SCENE_PROBE_SHARED_CACHE}
            --copy-resource ${EVE_SCENE_PROBE_SPRITE_STARS_RESOURCE} ${EVE_SCENE_PROBE_SPRITE_STARS}
            --copy-resource ${EVE_SCENE_PROBE_STAR_ATLAS_RESOURCE} ${EVE_SCENE_PROBE_STAR_ATLAS}
            --copy-resource ${EVE_SCENE_PROBE_STAR_COLORS_RESOURCE} ${EVE_SCENE_PROBE_STAR_COLORS}
            --copy-resource ${EVE_SCENE_PROBE_STAR_SPRITES_HI_RESOURCE} ${EVE_SCENE_PROBE_STAR_SPRITES_HI}
            --copy-resource ${EVE_SCENE_PROBE_STAR_SPRITES_LO_RESOURCE} ${EVE_SCENE_PROBE_STAR_SPRITES_LO}
            --copy-resource ${EVE_SCENE_PROBE_STAR_SPRITES_DEPTH_RESOURCE} ${EVE_SCENE_PROBE_STAR_SPRITES_DEPTH}
            --copy-resource ${EVE_SCENE_PROBE_NEW_EDEN_SUN_RESOURCE} ${EVE_SCENE_PROBE_NEW_EDEN_SUN}
            --copy-resource ${EVE_SCENE_PROBE_NEW_EDEN_PLANET_RESOURCE} ${EVE_SCENE_PROBE_NEW_EDEN_PLANET}
            --manifest ${EVE_SCENE_PROBE_NEW_EDEN_MANIFEST}
            --summary
        DEPENDS
            TrinitySharedCacheToGltf
            TrinityEveSceneProbeA01Assets
            ${EVE_SCENE_PROBE_SHARED_CACHE_INDEX}
            ${EVE_SCENE_PROBE_SHARED_CACHE_MACOS_INDEX}
        VERBATIM
    )
    add_custom_target(TrinityEveSceneProbeNewEdenAssets
        DEPENDS
            ${EVE_SCENE_PROBE_SPRITE_STARS}
            ${EVE_SCENE_PROBE_STAR_ATLAS}
            ${EVE_SCENE_PROBE_STAR_COLORS}
            ${EVE_SCENE_PROBE_STAR_SPRITES_HI}
            ${EVE_SCENE_PROBE_STAR_SPRITES_LO}
            ${EVE_SCENE_PROBE_STAR_SPRITES_DEPTH}
            ${EVE_SCENE_PROBE_NEW_EDEN_SUN}
            ${EVE_SCENE_PROBE_NEW_EDEN_PLANET}
            ${EVE_SCENE_PROBE_NEW_EDEN_MANIFEST}
    )
    add_dependencies(TrinityEveSceneProbeAssets TrinityEveSceneProbeNewEdenAssets)
endif()

if(EXISTS "${EVE_SCENE_PROBE_SHIP_SOURCE}")
    add_custom_command(
        OUTPUT
            ${EVE_SCENE_PROBE_SHIP_CMF}
            ${EVE_SCENE_PROBE_SHIP_BASECOLOR_RGBA}
            ${EVE_SCENE_PROBE_SHIP_BASECOLOR_METADATA}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${EVE_SCENE_PROBE_ASSETS_DIR}
        COMMAND $<TARGET_FILE:TrinityGltfToCmf> --input ${EVE_SCENE_PROBE_SHIP_SOURCE} --output ${EVE_SCENE_PROBE_SHIP_CMF} --summary
        DEPENDS TrinityGltfToCmf ${EVE_SCENE_PROBE_SHIP_SOURCE}
        VERBATIM
    )

    add_custom_target(TrinityEveSceneProbeShipAsset
        DEPENDS
            ${EVE_SCENE_PROBE_SHIP_CMF}
            ${EVE_SCENE_PROBE_SHIP_BASECOLOR_RGBA}
            ${EVE_SCENE_PROBE_SHIP_BASECOLOR_METADATA}
    )
    add_dependencies(TrinityEveSceneProbeAssets TrinityEveSceneProbeShipAsset)
endif()

if(EXISTS "${EVE_SCENE_PROBE_SHARED_CACHE_INDEX}")
    add_custom_command(
        OUTPUT
            ${EVE_SCENE_PROBE_ASTERO_GLTF}
            ${EVE_SCENE_PROBE_ASTERO_BIN}
            ${EVE_SCENE_PROBE_ASTERO_DDS}
            ${EVE_SCENE_PROBE_ASTERO_NORMAL_DDS}
            ${EVE_SCENE_PROBE_ASTERO_ROUGHNESS_DDS}
            ${EVE_SCENE_PROBE_ASTERO_MATERIAL_DDS}
            ${EVE_SCENE_PROBE_ASTERO_GLOW_DDS}
            ${EVE_SCENE_PROBE_ASTERO_DETAIL_DDS}
            ${EVE_SCENE_PROBE_ASTERO_MASK_DDS}
            ${EVE_SCENE_PROBE_ASTERO_PAINT_DDS}
            ${EVE_SCENE_PROBE_ASTERO_MODEL_MANIFEST}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${EVE_SCENE_PROBE_ASSETS_DIR}
        COMMAND ${TRINITY_SHARED_CACHE_NODE_TOOL} ${TRINITY_SHARED_CACHE_TO_GLTF_SCRIPT}
            --cache-root ${EVE_SCENE_PROBE_SHARED_CACHE}
            --model ${EVE_SCENE_PROBE_ASTERO_MODEL}
            --output ${EVE_SCENE_PROBE_ASTERO_GLTF}
            --base-color ${EVE_SCENE_PROBE_ASTERO_BASE_COLOR}
            --base-color-output ${EVE_SCENE_PROBE_ASTERO_DDS}
            --include-groups 0,1,2
            --copy-resource ${EVE_SCENE_PROBE_ASTERO_NORMAL} ${EVE_SCENE_PROBE_ASTERO_NORMAL_DDS}
            --copy-resource ${EVE_SCENE_PROBE_ASTERO_ROUGHNESS} ${EVE_SCENE_PROBE_ASTERO_ROUGHNESS_DDS}
            --copy-resource ${EVE_SCENE_PROBE_ASTERO_MATERIAL} ${EVE_SCENE_PROBE_ASTERO_MATERIAL_DDS}
            --copy-resource ${EVE_SCENE_PROBE_ASTERO_GLOW} ${EVE_SCENE_PROBE_ASTERO_GLOW_DDS}
            --copy-resource ${EVE_SCENE_PROBE_ASTERO_DETAIL} ${EVE_SCENE_PROBE_ASTERO_DETAIL_DDS}
            --copy-resource ${EVE_SCENE_PROBE_ASTERO_MASK} ${EVE_SCENE_PROBE_ASTERO_MASK_DDS}
            --copy-resource ${EVE_SCENE_PROBE_ASTERO_PAINT} ${EVE_SCENE_PROBE_ASTERO_PAINT_DDS}
            --manifest ${EVE_SCENE_PROBE_ASTERO_MODEL_MANIFEST}
            --summary
        DEPENDS
            TrinitySharedCacheToGltf
            ${TRINITY_SHARED_CACHE_TO_GLTF_SCRIPT}
            ${EVE_SCENE_PROBE_SHARED_CACHE_INDEX}
        VERBATIM
    )

    add_custom_command(
        OUTPUT ${EVE_SCENE_PROBE_ASTERO_CMF}
        COMMAND $<TARGET_FILE:TrinityGltfToCmf>
            --input ${EVE_SCENE_PROBE_ASTERO_GLTF}
            --output ${EVE_SCENE_PROBE_ASTERO_CMF}
            --summary
        DEPENDS
            TrinityGltfToCmf
            ${EVE_SCENE_PROBE_ASTERO_GLTF}
            ${EVE_SCENE_PROBE_ASTERO_BIN}
        VERBATIM
    )

    add_eve_scene_probe_dds_conversion(${EVE_SCENE_PROBE_ASTERO_DDS} ${EVE_SCENE_PROBE_ASTERO_BASECOLOR_RGBA} ${EVE_SCENE_PROBE_ASTERO_BASECOLOR_METADATA})
    add_eve_scene_probe_dds_conversion(${EVE_SCENE_PROBE_ASTERO_NORMAL_DDS} ${EVE_SCENE_PROBE_ASTERO_NORMAL_RGBA} ${EVE_SCENE_PROBE_ASTERO_NORMAL_METADATA})
    add_eve_scene_probe_dds_conversion(${EVE_SCENE_PROBE_ASTERO_ROUGHNESS_DDS} ${EVE_SCENE_PROBE_ASTERO_ROUGHNESS_RGBA} ${EVE_SCENE_PROBE_ASTERO_ROUGHNESS_METADATA})
    add_eve_scene_probe_dds_conversion(${EVE_SCENE_PROBE_ASTERO_MATERIAL_DDS} ${EVE_SCENE_PROBE_ASTERO_MATERIAL_RGBA} ${EVE_SCENE_PROBE_ASTERO_MATERIAL_METADATA})
    add_eve_scene_probe_dds_conversion(${EVE_SCENE_PROBE_ASTERO_GLOW_DDS} ${EVE_SCENE_PROBE_ASTERO_GLOW_RGBA} ${EVE_SCENE_PROBE_ASTERO_GLOW_METADATA})
    add_eve_scene_probe_dds_conversion(${EVE_SCENE_PROBE_ASTERO_DETAIL_DDS} ${EVE_SCENE_PROBE_ASTERO_DETAIL_RGBA} ${EVE_SCENE_PROBE_ASTERO_DETAIL_METADATA})
    add_eve_scene_probe_dds_conversion(${EVE_SCENE_PROBE_ASTERO_MASK_DDS} ${EVE_SCENE_PROBE_ASTERO_MASK_RGBA} ${EVE_SCENE_PROBE_ASTERO_MASK_METADATA})
    add_eve_scene_probe_dds_conversion(${EVE_SCENE_PROBE_ASTERO_PAINT_DDS} ${EVE_SCENE_PROBE_ASTERO_PAINT_RGBA} ${EVE_SCENE_PROBE_ASTERO_PAINT_METADATA})

    add_custom_target(TrinityEveSceneProbeAsteroAsset
        DEPENDS
            ${EVE_SCENE_PROBE_ASTERO_CMF}
            ${EVE_SCENE_PROBE_ASTERO_BASECOLOR_RGBA}
            ${EVE_SCENE_PROBE_ASTERO_BASECOLOR_METADATA}
            ${EVE_SCENE_PROBE_ASTERO_NORMAL_RGBA}
            ${EVE_SCENE_PROBE_ASTERO_NORMAL_METADATA}
            ${EVE_SCENE_PROBE_ASTERO_ROUGHNESS_RGBA}
            ${EVE_SCENE_PROBE_ASTERO_ROUGHNESS_METADATA}
            ${EVE_SCENE_PROBE_ASTERO_MATERIAL_RGBA}
            ${EVE_SCENE_PROBE_ASTERO_MATERIAL_METADATA}
            ${EVE_SCENE_PROBE_ASTERO_GLOW_RGBA}
            ${EVE_SCENE_PROBE_ASTERO_GLOW_METADATA}
            ${EVE_SCENE_PROBE_ASTERO_DETAIL_RGBA}
            ${EVE_SCENE_PROBE_ASTERO_DETAIL_METADATA}
            ${EVE_SCENE_PROBE_ASTERO_MASK_RGBA}
            ${EVE_SCENE_PROBE_ASTERO_MASK_METADATA}
            ${EVE_SCENE_PROBE_ASTERO_PAINT_RGBA}
            ${EVE_SCENE_PROBE_ASTERO_PAINT_METADATA}
            ${EVE_SCENE_PROBE_ASTERO_MODEL_MANIFEST}
    )
    add_dependencies(TrinityEveSceneProbeAssets TrinityEveSceneProbeAsteroAsset)
endif()

if(TARGET TrinityAnimatedModelAssets)
    set(EVE_SCENE_PROBE_FOX_CMF ${CMAKE_BINARY_DIR}/samples/animated_model/Assets/Fox.cmf)
    add_dependencies(TrinityEveSceneProbeAssets TrinityAnimatedModelAssets)
endif()

ccp_add_executable(TrinityALEveSceneProbe_metal
    EveSceneProbe.mm
)

add_dependencies(TrinityALEveSceneProbe_metal TrinityEveSceneProbeAssets)

target_compile_definitions(TrinityALEveSceneProbe_metal
    PRIVATE
        TRINITY_PLATFORM=TRINITY_METAL
        TRINITY_AL_WITH_BLUE_EXPOSURE=1
        BLUE_OVERRIDE_VECTOR_TYPES=1
        TBB_ENABLED=1
        DEPRECATED_ENABLED=1
)

target_include_directories(TrinityALEveSceneProbe_metal
    PRIVATE
        ${CMAKE_SOURCE_DIR}/trinity
        ${CMAKE_SOURCE_DIR}/trinity/include
        ${CMAKE_SOURCE_DIR}/trinityal
        ${CMAKE_SOURCE_DIR}/trinityal/include
)

target_link_libraries(TrinityALEveSceneProbe_metal
    PRIVATE
        trinity_metal
        "-framework Cocoa"
        "-framework ImageIO"
        "-framework QuartzCore"
        "-framework UniformTypeIdentifiers"
)

target_compile_options(TrinityALEveSceneProbe_metal
    PRIVATE
        "-fobjc-arc"
        "-fdeclspec"
        "-Wno-nonportable-include-path"
        "-Wno-inconsistent-missing-override"
)

set_target_properties(TrinityALEveSceneProbe_metal
    PROPERTIES
        FOLDER Trinity/Samples
        LINKER_LANGUAGE OBJCXX
        BUILD_RPATH "$<TARGET_FILE_DIR:trinity_metal>;${EVE_SCENE_PROBE_VCPKG_LIB_DIR}"
)

set_property(TARGET TrinityALEveSceneProbe_metal APPEND PROPERTY LINK_DEPENDS
	${EVE_SCENE_PROBE_EFFECT}
	${EVE_SCENE_PROBE_EFFECT_DEPTH}
	${EVE_SCENE_PROBE_RENDER_PRODUCT_EFFECT}
	${EVE_SCENE_PROBE_RENDER_PRODUCT_EFFECT_DEPTH}
)

if(TARGET TrinityEveSceneProbePostProcessEffects)
    set_property(TARGET TrinityALEveSceneProbe_metal APPEND PROPERTY LINK_DEPENDS
        ${EVE_SCENE_PROBE_TONEMAPPING_EFFECT}
        ${EVE_SCENE_PROBE_TONEMAPPING_EFFECT_DEPTH}
        ${EVE_SCENE_PROBE_CREATE_HISTOGRAMS_EFFECT}
        ${EVE_SCENE_PROBE_CREATE_HISTOGRAMS_EFFECT_DEPTH}
        ${EVE_SCENE_PROBE_MERGE_HISTOGRAMS_EFFECT}
        ${EVE_SCENE_PROBE_MERGE_HISTOGRAMS_EFFECT_DEPTH}
        ${EVE_SCENE_PROBE_MEASURE_EXPOSURE_EFFECT}
        ${EVE_SCENE_PROBE_MEASURE_EXPOSURE_EFFECT_DEPTH}
        ${EVE_SCENE_PROBE_BLIT_EFFECT}
        ${EVE_SCENE_PROBE_BLIT_EFFECT_DEPTH}
        ${EVE_SCENE_PROBE_BLIT_FILTERED_EFFECT}
        ${EVE_SCENE_PROBE_BLIT_FILTERED_EFFECT_DEPTH}
        ${EVE_SCENE_PROBE_BLACK_TEXTURE}
        ${EVE_SCENE_PROBE_REFLECTION_TEXTURE}
        ${EVE_SCENE_PROBE_FLAT_NORMAL_TEXTURE}
        ${EVE_SCENE_PROBE_REFLECTION_CORRECTION_TEXTURE}
        ${EVE_SCENE_PROBE_POSTPROCESS_MANIFEST}
    )
endif()

if(TARGET TrinityEveSceneProbeClientAssets)
    set_property(TARGET TrinityALEveSceneProbe_metal APPEND PROPERTY LINK_DEPENDS
        ${EVE_SCENE_PROBE_ASTERO_HULL_BLACK}
        ${EVE_SCENE_PROBE_ASTERO_FACTION_BLACK}
        ${EVE_SCENE_PROBE_CHROME_MATERIAL_BLACK}
        ${EVE_SCENE_PROBE_STEEL_MATERIAL_BLACK}
        ${EVE_SCENE_PROBE_WHITE_MATERIAL_BLACK}
        ${EVE_SCENE_PROBE_RED_MATERIAL_BLACK}
        ${EVE_SCENE_PROBE_GLASS_MATERIAL_BLACK}
        ${EVE_SCENE_PROBE_QUADV5_HI}
        ${EVE_SCENE_PROBE_QUADV5_LO}
        ${EVE_SCENE_PROBE_QUADV5_DEPTH}
        ${EVE_SCENE_PROBE_QUADHEATV5_HI}
        ${EVE_SCENE_PROBE_QUADHEATV5_LO}
        ${EVE_SCENE_PROBE_QUADHEATV5_DEPTH}
        ${EVE_SCENE_PROBE_FXDISTORTIONV5_HI}
        ${EVE_SCENE_PROBE_FXDISTORTIONV5_LO}
        ${EVE_SCENE_PROBE_FXDISTORTIONV5_DEPTH}
        ${EVE_SCENE_PROBE_WARP_DISTORTION}
        ${EVE_SCENE_PROBE_FITTING_SCENE}
        ${EVE_SCENE_PROBE_FITTING_CUBE}
        ${EVE_SCENE_PROBE_FITTING_CUBE_BLUR}
        ${EVE_SCENE_PROBE_FITTING_CUBE_REFL}
        ${EVE_SCENE_PROBE_DEFAULT_POSTPROCESS}
        ${EVE_SCENE_PROBE_CLIENT_MANIFEST}
    )
    add_custom_command(
        TARGET TrinityALEveSceneProbe_metal
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/model/spaceobjectfactory/hulls
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/model/spaceobjectfactory/factions
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/model/spaceobjectfactory/materials
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/spaceobject/v5/quad
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/spaceobject/v5/fx
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/texture/sprite
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/scene/fitting
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/default
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_ASTERO_HULL_BLACK} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/model/spaceobjectfactory/hulls/soef1_t1.black
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_ASTERO_FACTION_BLACK} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/model/spaceobjectfactory/factions/soebase.black
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_CHROME_MATERIAL_BLACK} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/model/spaceobjectfactory/materials/chrome_metallic.black
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_STEEL_MATERIAL_BLACK} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/model/spaceobjectfactory/materials/grey_steel_brushed.black
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_WHITE_MATERIAL_BLACK} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/model/spaceobjectfactory/materials/white_ghost_matt.black
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_RED_MATERIAL_BLACK} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/model/spaceobjectfactory/materials/red_crimson_enamel.black
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_GLASS_MATERIAL_BLACK} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/model/spaceobjectfactory/materials/glass_soe.black
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_QUADV5_HI} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/spaceobject/v5/quad/quadv5.sm_hi
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_QUADV5_LO} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/spaceobject/v5/quad/quadv5.sm_lo
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_QUADV5_DEPTH} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/spaceobject/v5/quad/quadv5.sm_depth
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_QUADHEATV5_HI} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/spaceobject/v5/quad/quadheatv5.sm_hi
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_QUADHEATV5_LO} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/spaceobject/v5/quad/quadheatv5.sm_lo
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_QUADHEATV5_DEPTH} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/spaceobject/v5/quad/quadheatv5.sm_depth
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_FXDISTORTIONV5_HI} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/spaceobject/v5/fx/fxdistortionv5.sm_hi
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_FXDISTORTIONV5_LO} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/spaceobject/v5/fx/fxdistortionv5.sm_lo
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_FXDISTORTIONV5_DEPTH} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/spaceobject/v5/fx/fxdistortionv5.sm_depth
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_WARP_DISTORTION} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/texture/sprite/warptunnel4_n.dds
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_FITTING_SCENE} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/scene/fitting/fitting.black
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_FITTING_CUBE} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/scene/fitting/fitting_cube.dds
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_FITTING_CUBE_BLUR} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/scene/fitting/fitting_cube_blur.dds
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_FITTING_CUBE_REFL} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/scene/fitting/fitting_cube_refl.dds
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_DEFAULT_POSTPROCESS} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/default/postprocess.black
        VERBATIM
    )
endif()

if(TARGET TrinityEveSceneProbeA01Assets)
    set_property(TARGET TrinityALEveSceneProbe_metal APPEND PROPERTY LINK_DEPENDS
        ${EVE_SCENE_PROBE_A01_SCENE}
        ${EVE_SCENE_PROBE_A01_CUBE}
        ${EVE_SCENE_PROBE_A01_REFLECTION}
        ${EVE_SCENE_PROBE_A01_BLUR}
        ${EVE_SCENE_PROBE_A01_LOW_DETAIL}
        ${EVE_SCENE_PROBE_A01_LOW_QUALITY}
        ${EVE_SCENE_PROBE_A01_LOW_QUALITY_MIX}
        ${EVE_SCENE_PROBE_A01_OVERLAY}
        ${EVE_SCENE_PROBE_A01_STARS}
        ${EVE_SCENE_PROBE_BACKGROUND_HI}
        ${EVE_SCENE_PROBE_BACKGROUND_LO}
        ${EVE_SCENE_PROBE_BACKGROUND_DEPTH}
        ${EVE_SCENE_PROBE_A01_MANIFEST}
    )
    add_custom_command(
        TARGET TrinityALEveSceneProbe_metal
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/scene/universe/lowquality
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/scene/starfield
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/texture/fx/misc
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/specialfx
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_A01_SCENE} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/scene/universe/a01_cube.black
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_A01_CUBE} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/scene/universe/a01_cube.dds
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_A01_REFLECTION} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/scene/universe/a01_cube_refl.dds
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_A01_BLUR} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/scene/universe/a01_cube_blur.dds
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_A01_LOW_DETAIL} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/scene/universe/a01_cube_lowdetail.dds
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_A01_LOW_QUALITY} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/scene/universe/lowquality/wormhole_a01.dds
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_A01_LOW_QUALITY_MIX} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/scene/universe/lowquality/wormhole_a01_mix.dds
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_A01_OVERLAY} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/texture/fx/misc/rgb_01a.dds
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_A01_STARS} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/scene/starfield/stars01_tile2.dds
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_BACKGROUND_HI} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/specialfx/background.sm_hi
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_BACKGROUND_LO} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/specialfx/background.sm_lo
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_BACKGROUND_DEPTH} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/specialfx/background.sm_depth
        VERBATIM
    )
endif()

if(TARGET TrinityEveSceneProbeNewEdenAssets)
    set_property(TARGET TrinityALEveSceneProbe_metal APPEND PROPERTY LINK_DEPENDS
        ${EVE_SCENE_PROBE_SPRITE_STARS}
        ${EVE_SCENE_PROBE_STAR_ATLAS}
        ${EVE_SCENE_PROBE_STAR_COLORS}
        ${EVE_SCENE_PROBE_STAR_SPRITES_HI}
        ${EVE_SCENE_PROBE_STAR_SPRITES_LO}
        ${EVE_SCENE_PROBE_STAR_SPRITES_DEPTH}
        ${EVE_SCENE_PROBE_NEW_EDEN_MANIFEST}
    )
    add_custom_command(
        TARGET TrinityALEveSceneProbe_metal
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/scene/starfield
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/specialfx
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_SPRITE_STARS} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/scene/starfield/spritestars.black
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_STAR_ATLAS} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/scene/starfield/staratlas.dds
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_STAR_COLORS} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/scene/starfield/starcolors.dds
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_STAR_SPRITES_HI} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/specialfx/starsprites.sm_hi
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_STAR_SPRITES_LO} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/specialfx/starsprites.sm_lo
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_STAR_SPRITES_DEPTH} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/specialfx/starsprites.sm_depth
        VERBATIM
    )
endif()

if(TARGET TrinityEveSceneProbeLocalLightAssets)
    set_property(TARGET TrinityALEveSceneProbe_metal APPEND PROPERTY LINK_DEPENDS
        ${EVE_SCENE_PROBE_COMPUTE_LIGHT_LISTS_HI}
        ${EVE_SCENE_PROBE_COMPUTE_LIGHT_LISTS_LO}
        ${EVE_SCENE_PROBE_COMPUTE_LIGHT_LISTS_DEPTH}
        ${EVE_SCENE_PROBE_LOCAL_LIGHT_BANNER_BLACK}
        ${EVE_SCENE_PROBE_LOCAL_LIGHT_SOE_BANNER}
        ${EVE_SCENE_PROBE_LOCAL_LIGHT_MANIFEST}
    )
    add_custom_command(
        TARGET TrinityALEveSceneProbe_metal
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/system
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/texture/global
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/ui/texture/classes/banners/factional/various
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_COMPUTE_LIGHT_LISTS_HI} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/system/computelightlists.sm_hi
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_COMPUTE_LIGHT_LISTS_LO} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/system/computelightlists.sm_lo
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_COMPUTE_LIGHT_LISTS_DEPTH} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/system/computelightlists.sm_depth
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_LOCAL_LIGHT_BANNER_BLACK} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/texture/global/black.dds
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_LOCAL_LIGHT_SOE_BANNER} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/ui/texture/classes/banners/factional/various/soe_banner_base.dds
        VERBATIM
    )
endif()

add_custom_command(
    TARGET TrinityALEveSceneProbe_metal
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/spaceobject/probe
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:trinity_metal> $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/$<TARGET_FILE_NAME:trinity_metal>
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_EFFECT} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/spaceobject/probe/asteropbr.sm_hi
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_EFFECT_DEPTH} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/spaceobject/probe/asteropbr.sm_depth
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_RENDER_PRODUCT_EFFECT} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/spaceobject/probe/renderproductvisualizer.sm_hi
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_RENDER_PRODUCT_EFFECT_DEPTH} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/spaceobject/probe/renderproductvisualizer.sm_depth
	VERBATIM
)

if(TARGET TrinityEveSceneProbePostProcessEffects)
    add_custom_command(
        TARGET TrinityALEveSceneProbe_metal
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/postprocess
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/system
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/texture/global
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_TONEMAPPING_EFFECT} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/postprocess/tonemapping.sm_hi
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_TONEMAPPING_EFFECT_DEPTH} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/postprocess/tonemapping.sm_depth
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_CREATE_HISTOGRAMS_EFFECT} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/postprocess/createhistograms.sm_hi
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_CREATE_HISTOGRAMS_EFFECT_DEPTH} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/postprocess/createhistograms.sm_depth
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_MERGE_HISTOGRAMS_EFFECT} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/postprocess/mergehistograms.sm_hi
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_MERGE_HISTOGRAMS_EFFECT_DEPTH} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/postprocess/mergehistograms.sm_depth
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_MEASURE_EXPOSURE_EFFECT} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/postprocess/measureexposure.sm_hi
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_MEASURE_EXPOSURE_EFFECT_DEPTH} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/postprocess/measureexposure.sm_depth
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_BLIT_EFFECT} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/system/blit.sm_hi
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_BLIT_EFFECT_DEPTH} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/system/blit.sm_depth
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_BLIT_FILTERED_EFFECT} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/system/blitfiltered.sm_hi
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_BLIT_FILTERED_EFFECT_DEPTH} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/graphics/effect.metal/managed/space/system/blitfiltered.sm_depth
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_BLACK_TEXTURE} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/texture/global/black.dds
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_REFLECTION_TEXTURE} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/texture/global/genericreflection_cube.dds
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_FLAT_NORMAL_TEXTURE} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/texture/global/flatnormal.dds
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_REFLECTION_CORRECTION_TEXTURE} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/texture/reflectioncorrection/128x128.dds
        VERBATIM
    )
endif()

if(TARGET TrinityEveSceneProbeShipAsset)
    add_custom_command(
        TARGET TrinityALEveSceneProbe_metal
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_SHIP_CMF} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/11989_lite.cmf
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_SHIP_BASECOLOR_RGBA} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/11989_lite.cmf.basecolor.rgba
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_SHIP_BASECOLOR_METADATA} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/11989_lite.cmf.basecolor.txt
        VERBATIM
    )
endif()

if(TARGET TrinityEveSceneProbeAsteroAsset)
    set_property(TARGET TrinityALEveSceneProbe_metal APPEND PROPERTY LINK_DEPENDS
        ${EVE_SCENE_PROBE_ASTERO_CMF}
        ${EVE_SCENE_PROBE_ASTERO_BASECOLOR_RGBA}
        ${EVE_SCENE_PROBE_ASTERO_BASECOLOR_METADATA}
        ${EVE_SCENE_PROBE_ASTERO_NORMAL_RGBA}
        ${EVE_SCENE_PROBE_ASTERO_NORMAL_METADATA}
        ${EVE_SCENE_PROBE_ASTERO_ROUGHNESS_RGBA}
        ${EVE_SCENE_PROBE_ASTERO_ROUGHNESS_METADATA}
        ${EVE_SCENE_PROBE_ASTERO_MATERIAL_RGBA}
        ${EVE_SCENE_PROBE_ASTERO_MATERIAL_METADATA}
        ${EVE_SCENE_PROBE_ASTERO_GLOW_RGBA}
        ${EVE_SCENE_PROBE_ASTERO_GLOW_METADATA}
        ${EVE_SCENE_PROBE_ASTERO_DETAIL_RGBA}
        ${EVE_SCENE_PROBE_ASTERO_DETAIL_METADATA}
        ${EVE_SCENE_PROBE_ASTERO_MASK_RGBA}
        ${EVE_SCENE_PROBE_ASTERO_MASK_METADATA}
        ${EVE_SCENE_PROBE_ASTERO_PAINT_RGBA}
        ${EVE_SCENE_PROBE_ASTERO_PAINT_METADATA}
    )
    add_custom_command(
        TARGET TrinityALEveSceneProbe_metal
        POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/model/ship/soe/frigate/soef1
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_ASTERO_CMF} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/Astero.cmf
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_ASTERO_DDS} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/model/ship/soe/frigate/soef1/soef1_t1_a.dds
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_ASTERO_NORMAL_DDS} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/model/ship/soe/frigate/soef1/soef1_t1_n.dds
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_ASTERO_ROUGHNESS_DDS} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/model/ship/soe/frigate/soef1/soef1_t1_r.dds
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_ASTERO_MATERIAL_DDS} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/model/ship/soe/frigate/soef1/soef1_t1_m.dds
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_ASTERO_GLOW_DDS} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/model/ship/soe/frigate/soef1/soef1_t1_g.dds
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_ASTERO_DETAIL_DDS} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/model/ship/soe/frigate/soef1/soef1_t1_d.dds
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_ASTERO_MASK_DDS} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/model/ship/soe/frigate/soef1/soef1_t1_mask.dds
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_ASTERO_PAINT_DDS} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/dx9/model/ship/soe/frigate/soef1/soef1_t1_p3.dds
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_ASTERO_BASECOLOR_RGBA} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/Astero.cmf.basecolor.rgba
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_ASTERO_BASECOLOR_METADATA} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/Astero.cmf.basecolor.txt
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_ASTERO_NORMAL_RGBA} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/Astero.cmf.normal.rgba
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_ASTERO_NORMAL_METADATA} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/Astero.cmf.normal.txt
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_ASTERO_ROUGHNESS_RGBA} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/Astero.cmf.roughness.rgba
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_ASTERO_ROUGHNESS_METADATA} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/Astero.cmf.roughness.txt
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_ASTERO_MATERIAL_RGBA} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/Astero.cmf.material.rgba
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_ASTERO_MATERIAL_METADATA} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/Astero.cmf.material.txt
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_ASTERO_GLOW_RGBA} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/Astero.cmf.glow.rgba
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_ASTERO_GLOW_METADATA} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/Astero.cmf.glow.txt
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_ASTERO_DETAIL_RGBA} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/Astero.cmf.d.rgba
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_ASTERO_DETAIL_METADATA} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/Astero.cmf.d.txt
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_ASTERO_MASK_RGBA} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/Astero.cmf.mask.rgba
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_ASTERO_MASK_METADATA} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/Astero.cmf.mask.txt
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_ASTERO_PAINT_RGBA} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/Astero.cmf.p3.rgba
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_ASTERO_PAINT_METADATA} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/Astero.cmf.p3.txt
        VERBATIM
    )
endif()

if(TARGET TrinityAnimatedModelAssets)
    add_custom_command(
        TARGET TrinityALEveSceneProbe_metal
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EVE_SCENE_PROBE_FOX_CMF} $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets/Fox.cmf
        VERBATIM
    )
endif()

add_custom_target(TrinityEveSceneProbeRuntimeAssets
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>/Assets
    DEPENDS TrinityALEveSceneProbe_metal TrinityEveSceneProbeAssets
    VERBATIM
)

add_custom_target(run_TrinityALEveSceneProbe_metal
	COMMAND $<TARGET_FILE:TrinityALEveSceneProbe_metal> --windowed 640x480 --frames 180 --quality-rung model --asset astero --material-mode eve-v5
    DEPENDS TrinityEveSceneProbeRuntimeAssets
    WORKING_DIRECTORY $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>
    USES_TERMINAL
)

if(TARGET TrinityEveSceneProbeClientAssets)
    set(EVE_SCENE_PROBE_INSPECTION_REPORT ${CMAKE_CURRENT_BINARY_DIR}/Reports/AsteroClientAssets.md)
    add_custom_target(inspect_TrinityEveAsteroAssets
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/Reports
        COMMAND $<TARGET_FILE:TrinityALEveSceneProbe_metal>
            --windowed 64x64
            --inspect-client-assets ${EVE_SCENE_PROBE_INSPECTION_REPORT}
        DEPENDS TrinityEveSceneProbeRuntimeAssets
        WORKING_DIRECTORY $<TARGET_FILE_DIR:TrinityALEveSceneProbe_metal>
        USES_TERMINAL
    )
endif()
