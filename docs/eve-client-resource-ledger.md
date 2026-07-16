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
- `/Users/rebecca/src/github.com/berkeleynerd/trinity/.cmake-build-arm64-osx-debug/samples/eve_scene_probe/Reports/PostFinishResources.json`

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

### Local-shadow inherited resources

`Reports/AsteroLocalShadowResources.json` records 13 checksummed inputs reused
by the native raster local-shadow path: the `quadv5`, `quadheatv5`, and
`computelightlists` Metal high/low/depth variants; `soef1_t1.gr2`;
`soef1_t1.black`; `soebase.black`; and the deterministic SoE logo. Each report
entry contains the logical path, the absolute hashed source below
`~/Library/Application Support/EVE Online/SharedCache/ResFiles/`, byte size,
SHA-256, and ignored build-tree destination. The atlas-to-mask resolver is
repository-owned shader source and adds no client payload.

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

### RC-11 post-finish resources

```text
res:/graphics/effect.metal/managed/space/postprocess/highpassfilter.sm_hi
res:/graphics/effect.metal/managed/space/postprocess/highpassfilter.sm_lo
res:/graphics/effect.metal/managed/space/postprocess/highpassfilter.sm_depth
res:/graphics/effect.metal/managed/space/postprocess/blur.sm_hi
res:/graphics/effect.metal/managed/space/postprocess/blur.sm_lo
res:/graphics/effect.metal/managed/space/postprocess/blur.sm_depth
res:/graphics/effect.metal/managed/space/postprocess/filmgrain.sm_hi
res:/graphics/effect.metal/managed/space/postprocess/filmgrain.sm_lo
res:/graphics/effect.metal/managed/space/postprocess/filmgrain.sm_depth
res:/texture/global/film_grain_noise.png
```

`Reports/PostFinishResources.json` also records inherited checksummed evidence
for `postprocess.black`, tone mapping, and exposure shaders. The installed
historical RC-11 client bytecode used to establish `newBloom=false` is
`/Users/rebecca/Library/Application Support/EVE Online/SharedCache/tq/EVE.app/Contents/Resources/build/code.ccp`
(30,935,325 bytes, SHA-256
`232a2c1552cd00d030e7b9f6bf1d4956673e3c1be85f07f4b19ebe19131fa67f`).
This is an archive-specific RC-11 snapshot, not the current PL-14G authority;
PL-14G pins `19ff580705e3800ccaba7cc03b7b8c988ab4095eafc36912f090896082311564`.

### RC-12A authored distortion resources

`Reports/AsteroDistortionResources.json` stages the native compositor and
records inherited Astero inputs without copying client payloads into source:

```text
res:/graphics/effect.metal/managed/space/postprocess/distortion.sm_hi
res:/graphics/effect.metal/managed/space/postprocess/distortion.sm_lo
res:/graphics/effect.metal/managed/space/postprocess/distortion.sm_depth
res:/graphics/effect.metal/managed/space/spaceobject/v5/fx/fxdistortionv5.sm_*
res:/dx9/model/ship/soe/frigate/soef1/soef1_t1.gr2
res:/dx9/model/spaceobjectfactory/hulls/soef1_t1.black
res:/dx9/model/spaceobjectfactory/factions/soebase.black
res:/texture/sprite/warptunnel4_n.dds
res:/dx9/model/ship/soe/frigate/soef1/soef1_t1_mask.dds
```

The authoritative indices are
`/Users/rebecca/Library/Application Support/EVE Online/SharedCache/tq/EVE.app/Contents/Resources/build/resfileindex_macOS.txt`
and `.../resfileindex.txt`; the generated manifest records every absolute
hashed `ResFiles/` source, byte size, SHA-256, and ignored destination. The
high compositor source is
`/Users/rebecca/Library/Application Support/EVE Online/SharedCache/ResFiles/91/91d41a4e03d2ae5f_e8217fbb46815c7f0c7638f187b4ac2b`
(SHA-256 `c887ac26e00220a28a732f7f75ce03c870edf80e99a8df305eaf627172ee9fc9`).
`Reports/AsteroDistortionGeneratedCmf.sha256` records the generated CMF hash
separately. The older `res:/fisfx/postprocess/distortion.black` is intentionally
excluded because it belongs to the legacy postprocess-job path.

### RC-12B volumetric resources

`Reports/VolumetricResources.json` records 38 resources staged only under the
ignored probe build tree. The authored local-volume fixture is rooted at:

```text
res:/fisfx/vdb/worldobjectcloud2/silkset_01/silk_01a_graybrown.black
res:/fisfx/vdb/worldobjectcloud2/silkset_01/silk_01a_density.vta
res:/fisfx/vdb/worldobjectcloud2/silkset_01/silk_01a_density_lowdetail.vta
res:/fisfx/vdb/worldobjectcloud2/silkset_01/silk_01a_temperature.vta
res:/fisfx/vdb/worldobjectcloud2/silkset_01/silk_01a_temperature_lowdetail.vta
```

Supporting inputs are `cloudbluenoise.dds`, `cloud1_cube.dds`, and
`ramp_vdb_color_04a.dds`. The staged Metal graph contains high, low, and depth
variants of `basiccloud`, `basiccloudreflectionproxy`, `blurvolumetric`,
`downsampledepth`, `volumeblit`, `applyfroxels`, `calculatefroxels`,
`filterfroxels`, `raymarchfroxels`, and `updatemieenvironmentmap`.

The authoritative index is:

```text
/Users/rebecca/Library/Application Support/EVE Online/SharedCache/tq/EVE.app/Contents/Resources/build/resfileindex.txt
```

The selected Silk Black resolves to:

```text
/Users/rebecca/Library/Application Support/EVE Online/SharedCache/ResFiles/6e/6e04d3820694ca9a_9961a786f682735500c71b2fded0921a
```

It is 4,699 bytes with SHA-256
`5928f40112f8fc79519193ce16026c9e1ade072ec5396b122c345cce39dcc655`.
The generated manifest carries the absolute hashed source, size, and checksum
for every other input. No listed payload is source controlled.

RC-12B1 uses the Black-authored `CloudColor2` endpoint
`(0.0578054, 0.0193824, 0.0159963, 0.1647059)` to replace the unavailable
client brightness bootstrap and selects high density `2.0`. These are runtime
settings, not staged payloads. Local VDB composition is accepted as capability
evidence; the New Eden fixture does not author this cloud. RC-12B2 global
froxel execution remains blocked by the documented AGX watchdog incident, so
its staged shaders remain resource evidence only.

RC-12B2 adds no source-controlled payload. `froxel_lab.py prepare` parses the
existing v15 high-tier containers without creating an `MTLDevice`, extracts
only these selected functions into the external run directory, and records
their source-container and metallib SHA-256 values:

```text
applyfroxels.sm_hi: mainVS/mainPS, permutation 0
updatemieenvironmentmap.sm_hi: mainCS, permutation 0
calculatefroxels.sm_hi: mainCS, permutation 0
filterfroxels.sm_hi: mainCS, permutation 0
raymarchfroxels.sm_hi: mainCS, permutations 0 and 1
```

Packages, ledgers, reports, symbols, and diagnostics live under
`~/Library/Logs/TrinityFroxelLab/runs/<run-id>/`. They are external evidence,
not redistributable assets. The device-free audit records each original
SharedCache-resolved container path, size, checksum, AIR resource signature,
constant-buffer range, and reflected threadgroup. See
`docs/froxel-investigation.md` and `docs/froxel-lab-operator-guide.md`.

## RC-05D Astero engine resources

`TrinityEveSceneProbeAsteroEngineAssets` resolves 18 external inputs through
the Tranquility indexes. The graph includes the SOE race Black, three Metal
variants each of `boostervolumetric`, `boosterglowanimated`, and
`volumetrictrails`, shape/gradient/noise textures, the inherited hull Black,
and `res:/dx9/model/ship/booster/volumetrictrail.gr2`. Representative absolute
sources are:

```text
/Users/rebecca/Library/Application Support/EVE Online/SharedCache/ResFiles/8f/8f063a9a8b4ee381_39ac532e847500f025ac11277f5ef235
/Users/rebecca/Library/Application Support/EVE Online/SharedCache/ResFiles/4c/4ce22ba77e1395d7_b902cc875dd42801d40ea9ecfd64a80f
/Users/rebecca/Library/Application Support/EVE Online/SharedCache/ResFiles/fe/fe06b70c077d30a3_20f671bacd1642e70b19c687d6a1f822
/Users/rebecca/Library/Application Support/EVE Online/SharedCache/ResFiles/67/6775ce43676c8f48_5e63e7b7e4a21eb4eef23a2737f5ede8
```

The ignored `Reports/AsteroEngineResources.json` records the full absolute
source, source index, byte size, SHA-256, and destination for every entry. The
generated trail CMF is recorded separately because it is derived output, not
a client payload. No SharedCache file or generated CMF is source controlled.

## RC-13 temporal resources

`TrinityEveSceneProbeTemporalAssets` stages only six current Metal containers
under the ignored build tree and writes
`Reports/AsteroTemporalResources.json`:

```text
res:/graphics/effect.metal/managed/space/postprocess/taa.sm_hi
res:/graphics/effect.metal/managed/space/postprocess/taa.sm_lo
res:/graphics/effect.metal/managed/space/postprocess/taa.sm_depth
res:/graphics/effect.metal/managed/space/postprocess/taacopy.sm_hi
res:/graphics/effect.metal/managed/space/postprocess/taacopy.sm_lo
res:/graphics/effect.metal/managed/space/postprocess/taacopy.sm_depth
```

The high TAA and TAA-copy sources resolve respectively to:

```text
/Users/rebecca/Library/Application Support/EVE Online/SharedCache/ResFiles/f6/f6274030f42af48c_b43c06b6d260676a89d24058ac986236
/Users/rebecca/Library/Application Support/EVE Online/SharedCache/ResFiles/71/71d620ab37108a3f_79b7bf6f2e4612be15a4938db5dd5e1a
```

Their SHA-256 values are
`fa319e0546d80a9d0ade6a6f4615cf3ab597d989a65f19747b7794d2606b477c`
and `265514e4e923d4519d6f8a56b90e3832bc1a539af414570a09b58362354e7d18`.
The manifest records absolute sources, sizes, and checksums for all variants.
Historical RC-11 policy evidence remains at
`/Users/rebecca/Library/Application Support/EVE Online/SharedCache/tq/EVE.app/Contents/Resources/build/code.ccp`
(SHA-256 `232a2c1552cd00d030e7b9f6bf1d4956673e3c1be85f07f4b19ebe19131fa67f`);
it is checksummed in place and is not copied. That hash is archive-specific;
the current PL-14G bytecode authority is
`19ff580705e3800ccaba7cc03b7b8c988ab4095eafc36912f090896082311564`.
The legacy `res:/fisfx/postprocess/taa.black` is unused because the current
client creates
`Tr2PPTaaEffect` through render-driver settings.

RC-13 closeout required no additional client payload. Atomic temporal reports
embed the existing manifest and `code.ccp` checksum, plus same-frame product
formats and raw hashes. The accepted CP-29 and edge/integrated reports, and the
fail-closed Silk/trail reports, remain under the ignored absolute directory:

```text
/Users/rebecca/src/github.com/berkeleynerd/trinity/.cmake-build-arm64-osx-debug/samples/eve_scene_probe/Captures/rc13-closeout/
```

The isolated failures are runtime-contract evidence: both cooldown maps are
uniform zero. They do not imply missing staged resources and do not authorize
a sample-owned reactive mask.
