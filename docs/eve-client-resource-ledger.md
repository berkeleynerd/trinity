# EVE Client Resource Ledger

This experimental sample does not commit EVE client payloads. It resolves the
logical resources below from the installed EVE Online SharedCache and stages
only the selected files under the local CMake build tree.

## Host source paths

- SharedCache root: `/Users/rebecca/Library/Application Support/EVE Online/SharedCache`
- Hashed payload root: `/Users/rebecca/Library/Application Support/EVE Online/SharedCache/ResFiles`
- General resource index: `/Users/rebecca/Library/Application Support/EVE Online/SharedCache/tq/EVE.app/Contents/Resources/build/resfileindex.txt`
- macOS Metal resource index: `/Users/rebecca/Library/Application Support/EVE Online/SharedCache/tq/EVE.app/Contents/Resources/build/resfileindex_macOS.txt`

The paths above are host-specific examples matching this bring-up machine. The
CMake cache variable `EVE_SCENE_PROBE_SHARED_CACHE` defaults to
`$HOME/Library/Application Support/EVE Online/SharedCache` so another checkout
can use its own launcher cache without changing source files.

## Resolution reports

The resolver writes these build-tree reports on this host:

- `/Users/rebecca/src/github.com/berkeleynerd/trinity/.cmake-build-arm64-osx-debug/samples/eve_scene_probe/Reports/AsteroModelResources.json`
- `/Users/rebecca/src/github.com/berkeleynerd/trinity/.cmake-build-arm64-osx-debug/samples/eve_scene_probe/Reports/AsteroClientResources.json`
- `/Users/rebecca/src/github.com/berkeleynerd/trinity/.cmake-build-arm64-osx-debug/samples/eve_scene_probe/Reports/PostProcessResources.json`
- `/Users/rebecca/src/github.com/berkeleynerd/trinity/.cmake-build-arm64-osx-debug/samples/eve_scene_probe/Reports/A01SceneResources.json`
- `/Users/rebecca/src/github.com/berkeleynerd/trinity/.cmake-build-arm64-osx-debug/samples/eve_scene_probe/Reports/NewEdenSceneResources.json`
- `/Users/rebecca/src/github.com/berkeleynerd/trinity/.cmake-build-arm64-osx-debug/samples/eve_scene_probe/Reports/AsteroLocalLightResources.json`
- `/Users/rebecca/src/github.com/berkeleynerd/trinity/.cmake-build-arm64-osx-debug/samples/eve_scene_probe/Reports/AsteroAttachmentResources.json`
- `/Users/rebecca/src/github.com/berkeleynerd/trinity/.cmake-build-arm64-osx-debug/samples/eve_scene_probe/Reports/AsteroShadowAoResources.json`

Every JSON entry records `logicalPath`, the relative and absolute source index,
the relative and absolute hashed `ResFiles` source, byte size, SHA-256, and the
absolute staged destination. This keeps exact physical filenames observable
without making transient launcher hashes part of source control.

## Logical resource paths

These are the complete EVE client logical paths currently requested by
`samples/eve_scene_probe/CMakeLists.txt`.

### Astero model and textures

```text
res:/dx9/model/ship/soe/frigate/soef1/soef1_t1.gr2
res:/dx9/model/ship/soe/frigate/soef1/soef1_t1_a.dds
res:/dx9/model/ship/soe/frigate/soef1/soef1_t1_d.dds
res:/dx9/model/ship/soe/frigate/soef1/soef1_t1_g.dds
res:/dx9/model/ship/soe/frigate/soef1/soef1_t1_m.dds
res:/dx9/model/ship/soe/frigate/soef1/soef1_t1_mask.dds
res:/dx9/model/ship/soe/frigate/soef1/soef1_t1_n.dds
res:/dx9/model/ship/soe/frigate/soef1/soef1_t1_p3.dds
res:/dx9/model/ship/soe/frigate/soef1/soef1_t1_r.dds
res:/dx9/model/shared/faction_logos/soe_logo.dds
```

### SOF, materials, and fitting scene

```text
res:/dx9/default/postprocess.black
res:/dx9/model/spaceobjectfactory/factions/soebase.black
res:/dx9/model/spaceobjectfactory/generic.black
res:/dx9/model/spaceobjectfactory/hulls/soef1_t1.black
res:/dx9/model/spaceobjectfactory/materials/chrome_metallic.black
res:/dx9/model/spaceobjectfactory/materials/glass_soe.black
res:/dx9/model/spaceobjectfactory/materials/grey_steel_brushed.black
res:/dx9/model/spaceobjectfactory/materials/red_crimson_enamel.black
res:/dx9/model/spaceobjectfactory/materials/white_ghost_matt.black
res:/dx9/scene/fitting/fitting.black
res:/dx9/scene/fitting/fitting_cube.dds
res:/dx9/scene/fitting/fitting_cube_blur.dds
res:/dx9/scene/fitting/fitting_cube_refl.dds
```

### Universe and starfield

```text
res:/dx9/scene/starfield/spritestars.black
res:/dx9/scene/starfield/staratlas.dds
res:/dx9/scene/starfield/starcolors.dds
res:/dx9/scene/starfield/stars01_tile2.dds
res:/dx9/model/celestial/sun/sun_yellow_small_01b.black
res:/dx9/model/worldobject/planet/sandstormplanet.black
res:/dx9/scene/universe/a01_cube.black
res:/dx9/scene/universe/a01_cube.dds
res:/dx9/scene/universe/a01_cube_blur.dds
res:/dx9/scene/universe/a01_cube_lowdetail.dds
res:/dx9/scene/universe/a01_cube_refl.dds
res:/dx9/scene/universe/lowquality/wormhole_a01.dds
res:/dx9/scene/universe/lowquality/wormhole_a01_mix.dds
res:/texture/fx/misc/rgb_01a.dds
```

### Space-object Metal effects

```text
res:/graphics/effect.metal/managed/space/spaceobject/fx/banner.sm_depth
res:/graphics/effect.metal/managed/space/spaceobject/fx/banner.sm_hi
res:/graphics/effect.metal/managed/space/spaceobject/fx/banner.sm_lo
res:/graphics/effect.metal/managed/space/spaceobject/fx/blinkinglightspool.sm_depth
res:/graphics/effect.metal/managed/space/spaceobject/fx/blinkinglightspool.sm_hi
res:/graphics/effect.metal/managed/space/spaceobject/fx/blinkinglightspool.sm_lo
res:/graphics/effect.metal/managed/space/spaceobject/fx/hazespherical.sm_depth
res:/graphics/effect.metal/managed/space/spaceobject/fx/hazespherical.sm_hi
res:/graphics/effect.metal/managed/space/spaceobject/fx/hazespherical.sm_lo
res:/graphics/effect.metal/managed/space/spaceobject/fx/planeglow.sm_depth
res:/graphics/effect.metal/managed/space/spaceobject/fx/planeglow.sm_hi
res:/graphics/effect.metal/managed/space/spaceobject/fx/planeglow.sm_lo
res:/graphics/effect.metal/managed/space/spaceobject/fx/spotlightconepool.sm_depth
res:/graphics/effect.metal/managed/space/spaceobject/fx/spotlightconepool.sm_hi
res:/graphics/effect.metal/managed/space/spaceobject/fx/spotlightconepool.sm_lo
res:/graphics/effect.metal/managed/space/spaceobject/fx/spotlightglowpool.sm_depth
res:/graphics/effect.metal/managed/space/spaceobject/fx/spotlightglowpool.sm_hi
res:/graphics/effect.metal/managed/space/spaceobject/fx/spotlightglowpool.sm_lo
res:/graphics/effect.metal/managed/space/spaceobject/v5/fx/fxdistortionv5.sm_depth
res:/graphics/effect.metal/managed/space/spaceobject/v5/fx/fxdistortionv5.sm_hi
res:/graphics/effect.metal/managed/space/spaceobject/v5/fx/fxdistortionv5.sm_lo
res:/graphics/effect.metal/managed/space/spaceobject/v5/quad/quadheatv5.sm_depth
res:/graphics/effect.metal/managed/space/spaceobject/v5/quad/quadheatv5.sm_hi
res:/graphics/effect.metal/managed/space/spaceobject/v5/quad/quadheatv5.sm_lo
res:/graphics/effect.metal/managed/space/spaceobject/v5/quad/quadv5.sm_depth
res:/graphics/effect.metal/managed/space/spaceobject/v5/quad/quadv5.sm_hi
res:/graphics/effect.metal/managed/space/spaceobject/v5/quad/quadv5.sm_lo
```

`SM_3_0_DEPTH` selects the `.sm_depth` files and is the client's high shader
tier. `SM_3_0_HI` selects `.sm_hi` and corresponds to the client medium tier.

### Scene and starfield Metal effects

```text
res:/graphics/effect.metal/managed/space/specialfx/background.sm_depth
res:/graphics/effect.metal/managed/space/specialfx/background.sm_hi
res:/graphics/effect.metal/managed/space/specialfx/background.sm_lo
res:/graphics/effect.metal/managed/space/specialfx/starsprites.sm_depth
res:/graphics/effect.metal/managed/space/specialfx/starsprites.sm_hi
res:/graphics/effect.metal/managed/space/specialfx/starsprites.sm_lo
```

### Postprocess and lighting Metal effects

```text
res:/graphics/effect.metal/managed/space/postprocess/createhistograms.sm_depth
res:/graphics/effect.metal/managed/space/postprocess/createhistograms.sm_hi
res:/graphics/effect.metal/managed/space/postprocess/measureexposure.sm_depth
res:/graphics/effect.metal/managed/space/postprocess/measureexposure.sm_hi
res:/graphics/effect.metal/managed/space/postprocess/mergehistograms.sm_depth
res:/graphics/effect.metal/managed/space/postprocess/mergehistograms.sm_hi
res:/graphics/effect.metal/managed/space/postprocess/tonemapping.sm_depth
res:/graphics/effect.metal/managed/space/postprocess/tonemapping.sm_hi
res:/graphics/effect.metal/managed/space/system/blit.sm_depth
res:/graphics/effect.metal/managed/space/system/blit.sm_hi
res:/graphics/effect.metal/managed/space/system/blitfiltered.sm_depth
res:/graphics/effect.metal/managed/space/system/blitfiltered.sm_hi
res:/graphics/effect.metal/managed/space/system/computelightlists.sm_depth
res:/graphics/effect.metal/managed/space/system/computelightlists.sm_hi
res:/graphics/effect.metal/managed/space/system/computelightlists.sm_lo
res:/graphics/effect.metal/managed/space/system/shadowdepth.sm_depth
res:/graphics/effect.metal/managed/space/system/shadowdepth.sm_hi
res:/graphics/effect.metal/managed/space/system/shadowdepth.sm_lo
res:/graphics/effect.metal/managed/space/system/estimatenoise.sm_depth
res:/graphics/effect.metal/managed/space/system/estimatenoise.sm_hi
res:/graphics/effect.metal/managed/space/system/estimatenoise.sm_lo
res:/graphics/effect.metal/managed/space/system/denoiseestimate.sm_depth
res:/graphics/effect.metal/managed/space/system/denoiseestimate.sm_hi
res:/graphics/effect.metal/managed/space/system/denoiseestimate.sm_lo
res:/graphics/effect.metal/managed/space/system/denoise1d.sm_depth
res:/graphics/effect.metal/managed/space/system/denoise1d.sm_hi
res:/graphics/effect.metal/managed/space/system/denoise1d.sm_lo
res:/graphics/effect.metal/managed/space/system/cortao/cortao.sm_depth
res:/graphics/effect.metal/managed/space/system/cortao/cortao.sm_hi
res:/graphics/effect.metal/managed/space/system/cortao/cortao.sm_lo
res:/graphics/effect.metal/managed/space/system/cortao/blur.sm_depth
res:/graphics/effect.metal/managed/space/system/cortao/blur.sm_hi
res:/graphics/effect.metal/managed/space/system/cortao/blur.sm_lo
res:/graphics/effect.metal/managed/space/system/ssao/ssao.sm_depth
res:/graphics/effect.metal/managed/space/system/ssao/ssao.sm_hi
res:/graphics/effect.metal/managed/space/system/ssao/ssao.sm_lo
```

### Reflection-probe resources

```text
res:/dx9/scene/cinematic/black_cube_refl.dds
res:/graphics/effect.metal/managed/space/system/reflection/copycube.sm_depth
res:/graphics/effect.metal/managed/space/system/reflection/copycube.sm_hi
res:/graphics/effect.metal/managed/space/system/reflection/copycube.sm_lo
res:/graphics/effect.metal/managed/space/system/reflection/reflectionfilteractivision128.sm_depth
res:/graphics/effect.metal/managed/space/system/reflection/reflectionfilteractivision128.sm_hi
res:/graphics/effect.metal/managed/space/system/reflection/reflectionfilteractivision128.sm_lo
res:/graphics/effect.metal/managed/space/system/reflection/reflectionfilteractivisionpre.sm_depth
res:/graphics/effect.metal/managed/space/system/reflection/reflectionfilteractivisionpre.sm_hi
res:/graphics/effect.metal/managed/space/system/reflection/reflectionfilteractivisionpre.sm_lo
```

`AsteroV5IndirectResources.json` also records the A01 reflection cube,
reflection-correction lookup, and six V5 opaque effects as inherited inputs.
The black cube is an explicit off control; dynamic mode never falls back to it.

### Indexed decal resources

```text
res:/graphics/effect.metal/managed/space/decals/v5/decalv5.sm_depth
res:/graphics/effect.metal/managed/space/decals/v5/decalv5.sm_hi
res:/graphics/effect.metal/managed/space/decals/v5/decalv5.sm_lo
res:/graphics/effect.metal/managed/space/decals/v5/decalcounterv5.sm_depth
res:/graphics/effect.metal/managed/space/decals/v5/decalcounterv5.sm_hi
res:/graphics/effect.metal/managed/space/decals/v5/decalcounterv5.sm_lo
res:/dx9/model/decal/soe/glow_killmarks_01_t.dds
res:/dx9/model/decal/soe/logo_soe_01_a.dds
res:/dx9/model/decal/soe/logo_soe_01_f.dds
res:/dx9/model/decal/soe/logo_soe_01_n.dds
res:/dx9/model/decal/soe/logo_soe_01_r.dds
res:/dx9/model/decal/soe/logo_soe_01_t.dds
res:/dx9/model/decal/soe/marking_302va1_02_a.dds
res:/dx9/model/decal/soe/marking_302va1_02_f.dds
res:/dx9/model/decal/soe/marking_302va1_02_n.dds
res:/dx9/model/decal/soe/marking_302va1_02_r.dds
res:/dx9/model/decal/soe/marking_302va1_02_t.dds
res:/dx9/model/decal/soe/marking_info01_01_a.dds
res:/dx9/model/decal/soe/marking_info01_01_f.dds
res:/dx9/model/decal/soe/marking_info01_01_n.dds
res:/dx9/model/decal/soe/marking_info01_01_r.dds
res:/dx9/model/decal/soe/marking_info01_01_t.dds
res:/dx9/model/decal/soe/marking_propeldynamics01_01_a.dds
res:/dx9/model/decal/soe/marking_propeldynamics01_01_f.dds
res:/dx9/model/decal/soe/marking_propeldynamics01_01_n.dds
res:/dx9/model/decal/soe/marking_propeldynamics01_01_r.dds
res:/dx9/model/decal/soe/marking_propeldynamics01_01_t.dds
res:/dx9/model/shared/dirtdetailnoise.dds
```

`Reports/AsteroDecalResources.json` records these 28 direct inputs plus nine
inherited hull, faction, generic, GR2, and parent-map resources. Every entry
contains the logical path, absolute SharedCache hashed source, byte size,
SHA-256, and staged build-tree destination. The report and payloads remain
outside source control.

### Supporting textures

```text
res:/texture/fx/hologram/hologram_interlace_p.dds
res:/texture/fx/hologram/hologram_noise.dds
res:/texture/fx/hologram/hologram_pulse.dds
res:/texture/global/black.dds
res:/texture/global/flatnormal.dds
res:/texture/global/genericreflection_cube.dds
res:/texture/global/spotramp.dds
res:/texture/particle/whitesharp.dds
res:/texture/reflectioncorrection/128x128.dds
res:/texture/ssao/24x24x16x16.dds
res:/texture/sprite/astroiddirt3.dds
res:/texture/sprite/caustics.dds
res:/texture/sprite/fluorecent.dds
res:/texture/sprite/spotlight4.dds
res:/texture/sprite/white2.dds
res:/texture/sprite/warptunnel4_n.dds
res:/ui/texture/classes/banners/factional/various/soe_banner_base.dds
```

The flat normal and reflection-correction lookup are staged diagnostics. The
former isolates authored normal-map response for accepted RC-07 captures; the
latter proves the driver's client lookup is available even though the selected
opaque V5 permutation does not reflect or consume it. The render-product
visualization effect is built from repository source and does not add an EVE
client resource to this ledger.

The square SoE logo is a deterministic faction fixture for the Astero's two
alliance-logo and two corporation-logo slots. The tall `soe_banner_base.dds`
texture is retained only as provenance for the rejected first experiment. The
installed client's identity-dependent logo service would otherwise populate
these slots; its no-identity fallback is `res:/texture/global/black.dds`.
