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
```

### SOF, materials, and fitting scene

```text
res:/dx9/default/postprocess.black
res:/dx9/model/spaceobjectfactory/factions/soebase.black
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
```

### Supporting textures

```text
res:/texture/global/black.dds
res:/texture/global/genericreflection_cube.dds
res:/texture/sprite/warptunnel4_n.dds
res:/ui/texture/classes/banners/factional/various/soe_banner_base.dds
```

The SoE banner is a deterministic faction fixture for the four active Astero
banner-light descriptors. The installed client's identity-dependent alliance
and corporation logo service would otherwise populate those slots; its
no-identity fallback is `res:/texture/global/black.dds`.
