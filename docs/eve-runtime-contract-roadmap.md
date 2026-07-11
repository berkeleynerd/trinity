# EVE Runtime-Contract Roadmap

This tracker separates two questions that the original quality ladder combined:

1. Does an individual Trinity/EVE capability execute on macOS Metal?
2. Does the composed renderer satisfy enough of the EVE runtime contract for a
   visually meaningful fidelity comparison?

Capability evidence is retained even when a later fidelity gate is incomplete.
No postprocess checkpoint can compensate for missing scene, object-lighting, or
render-product inputs.

External EVE payload provenance is maintained in
[`eve-client-resource-ledger.md`](eve-client-resource-ledger.md); no listed
client asset is committed to this repository.

## Status notation

| Mark | Meaning |
| --- | --- |
| Accepted | Contract and evidence gate passed. |
| Partial | Some real inputs are present, but the contract is incomplete. |
| Active | Current direct-path work. |
| Blocked | A named prerequisite has not been accepted. |
| Capability only | Machinery executes, but its visual output is not fidelity evidence. |

## Direct runtime-fidelity path

This table is the authoritative implementation order.

| ID | Runtime contract | Status | Depends on | Evidence required for acceptance |
| --- | --- | --- | --- | --- |
| RC-00 | Standalone host lifecycle and frame clock | Accepted | None | Finite and long-running samples exit cleanly; Trinity animation time advances without the Python scheduler. |
| RC-01 | Select a representative in-space client scene | Accepted | RC-00 | `res:/dx9/scene/universe/a01_cube.black` is the deterministic environment fixture; it is not presented as a named solar system. |
| RC-02 | Close the scene resource graph | Accepted | RC-01 | `Reports/A01SceneResources.json` records 12 staged Black, texture, and Metal-effect resources with index source, byte count, and SHA-256. Missing resources fail the build-time copy. |
| RC-03 | Render the in-space background | Accepted | RC-02 | Scene-only and Astero captures show the authored A01 nebula and star map through `EveSpaceScene`; serialized effects and textures are synchronously validated before rendering. |
| RC-04 | Validate scene per-frame lighting | Accepted | RC-03 | New Eden supplies an authored A01 environment plus star direction/color derived with the client algorithm at the Promised Land stargate. Ambient, environment rotation, reflection intensity, and bound resources are logged; the seeded sprite starfield renders. |
| RC-04B | Reconstruct visible New Eden celestials | Accepted | RC-07 | Native `EvePlanet` renders the authored sun, Sandstorm surface, additive atmosphere, and transparent cloud layer. Exact-system model view retains 60-degree/subpixel framing; derived sun and planet diagnostics report 240.000 pixels against 240.000 expected. CPython 2.7 cloud selection, separate checksummed atmosphere resources, four distinct layer captures, and 180-frame model/HDR/exposure runs pass. Flare and god rays remain separate effects. |
| RC-04C | Reconstruct New Eden sun effects | Accepted | RC-04B | Authored `SunFlares`, system graphic `1247` (`yellow_small.black`), native foreground/background occlusion, and depth-aware god rays execute in order. The 43-resource manifest is complete; off/flare/god-rays/all captures have distinct hashes; a 540-frame orbit and settled `hdr-exposure` run return zero. |
| RC-05 | Complete the ship object/material-area contract | Accepted | RC-02 | Groups 0/1/2 remain distinct CMF sections. Hull and booster render through authored `quadv5`/`quadheatv5` batches; distortion geometry/effect/maps are validated and explicitly deferred to RC-12. `AsteroClientAssets.md` classifies every per-object default and absent area. |
| RC-05B | Render authored visible SOF attachments | Accepted | RC-05 | The active `primary`/`soe` visibility groups produce 83 sprites, 4 spotlights, 16 planes, 2 hazes, and 4 banners through native attachment and quad-renderer paths. Every isolated family differs from off, a 540-frame orbit is stable, and lights remain independently selectable. |
| RC-05C | Render indexed SOF decal sets | Accepted | RC-05B | Eleven active decals submit through native `EveSpaceObjectDecal`: six standard markings (14 triangles), four SoE logos (12 triangles), and one kill counter (2 triangles). Isolated family captures, integrated HDR/exposure runs, and byte-identical decal-off/on depth, normal, reflection, shadow, atlas, AO, and bent-normal products pass. |
| RC-05D | Render authored booster and engine effects | Queued | RC-05B, RC-12B | Reconstruct Astero booster locators, throttle-driven glow and plume geometry, exhaust particles, and trails through native Trinity machinery. Validate additive/transparent/distortion composition, attachment during rotation, and an explicit velocity/TAA policy. |
| RC-06 | Supply object SH and local/secondary lighting | Accepted | RC-04, RC-05 | The probe uses the client's high `SM_3_0_DEPTH` tier. Exact New Eden sun/planet inputs correctly produce zero secondary SH after Trinity's cutoff; the synthetic control proves the same upload path. Six authored `primary`/`soe` haze/banner lights resolve and rotate with the ship, and local off/authored/validation captures are distinct. |
| RC-07 | Validate depth and normal products | Accepted | RC-05 | The driver publishes reverse-Z `D32_FLOAT` depth and `R10G10B10A2_UNORM` normals through named outputs. A sample-owned GPU visualizer produces coherent 180-frame Astero captures; authored and flat-normal controls preserve silhouette/camera while differing in expected surface detail. |
| RC-08 | Add sun shadows and AO | Accepted | RC-06, RC-07, RC-08A | Native cascades and CORTAO consume the accepted depth/normal products. Three cascades commit six V5 batches, all named products remain nonuniform, and the dynamic environment probe preserves readable detail in fully sun-shadowed regions without shadow-lightness or synthetic ambient. |
| RC-08A | Restore V5 indirect fill and reflection probes | Accepted | RC-03, RC-05, RC-07 | Ultra dynamic reflection capture renders all six faces per frame and checks every copy/filter stage. The resulting 256x256 eight-mip cube is nonblack and nonuniform; off/static/dynamic environment-only captures are distinct. High-tier V5 consumes all seven SH vectors and the environment cube with client SH intensities `3.14`. |
| RC-08B | Determine authored local-light shadow contract | Closed: client-parity N/A | RC-05C, RC-08 | The current macOS client defaults `useDynamicLightsShadows=false`, contains no client override, and ships 2,224 unique Metal libraries exposing the dynamic-shadow binding with all 2,224 compiled `readnone`. Auto mode resolves off; explicit atlas modes remain CP-21 diagnostics. |
| RC-09 | Accept complete HDR scene composition | Accepted | RC-08B | `PreTonemapColor` proves the complete canonical composition is finite, nonuniform, and retains HDR headroom. Four controls differ from full; exact inventory checks and 540-frame windowed/native relative-pacing gates pass. |
| RC-10 | Reaccept exposure and tone mapping | Accepted | RC-09 | The installed client Black and baked Uncharted2 Metal container are validated directly. Every histogram, exposure recurrence, CPU tone reference, controlled camera cut, matched off/client A/B, and integrated 540-frame composition gate passes. |
| RC-11 | Add bloom and film grain | Accepted | RC-10 | Installed-client bytecode selects legacy high-pass/blur bloom (`newBloom=false`). Exact Black values, every native pass, atomic bloom/post-tone/final readback, isolated off/bloom/grain captures, and the complete 180-frame composition gate pass. |
| RC-12 | Add distortion and volumetrics | Active | RC-11 | Authored distortion is accepted under RC-12A; volumetric/froxel effects remain active as RC-12B. |
| RC-12A | Compose authored Astero distortion | Accepted | RC-11 | One native 40-triangle V5 distortion batch writes a localized full-resolution vector map and runs the client scene-warp compositor before exposure/tone/finish. Matched off/on HDR and final hashes differ while depth, normal, reflection, shadow, atlas, AO, and bent-normal products remain byte-identical. |
| RC-13 | Add velocity and TAA | Blocked | RC-05D, RC-07, RC-12B | Current/previous camera, ship, attachment, and engine-effect transforms must produce valid velocity; TAA is accepted only after direct velocity visualization and engine-trail ghosting checks. |

## Capability evidence tracker

These checkpoints prove machinery, not necessarily visual fidelity.

| ID | Capability | Status | Evidence | Fidelity limitation |
| --- | --- | --- | --- | --- |
| CP-00 | TrinityAL Metal presentation | Accepted | Pyramid and finite-frame smoke runs. | Does not exercise Trinity scene resources. |
| CP-01 | Public glTF-to-CMF animation path | Accepted | Box and Fox model/animation captures. | Standalone CPU skinning and sample materials. |
| CP-02 | EVE driver shell and empty scene | Accepted | `shell` and `scene` runs return zero. | No scene content. |
| CP-03 | SharedCache Astero conversion | Accepted | All three GR2 groups retain independent index ranges; isolated hull, booster, and distortion-geometry captures are coherent. | Build-time conversion bypasses runtime GR2 loading. |
| CP-04 | Native EVE render batch | Accepted | `Main` and `Depth` techniques submit through `ITr2Renderable`. | Bridge is not a complete SOF-built `EveSpaceObject2`. |
| CP-05 | Authored V5 material | Accepted as capability | Client `quadv5` and `quadheatv5`, SOF material/heat constants, logical textures, and independent batches load successfully. | Distortion composition remains separately gated; indexed overlays are accepted under CP-20. |
| CP-06 | FP16 resolve, tone mapping, and blit | Accepted as capability | `hdr-blit` and `hdr-post` complete finite captures. | This historical isolated checkpoint is superseded by CP-22 composition and CP-23 tone fidelity. |
| CP-07 | Client dynamic exposure compute | Accepted as capability | `hdr-exposure` runs histogram, merge, measure, and tone passes for 180 frames. | Fidelity is reaccepted only after RC-09 closes decals and local-light shadows. |
| CP-08 | Fitting-scene illumination transport | Accepted as capability | Authored fitting sun, ambient, reflection intensity, and cube reach per-frame V5 constants. | Fitting is not representative in-space lighting and supplies no secondary SH sources. |
| CP-09 | Authored universe background | Accepted | A01 background effect, cube textures, star map, reflection cube, and low-quality resources load from a checksummed build manifest and survive model/HDR/exposure captures. | The fixture does not reconstruct a named system's sun object or seeded `EveStarfield`. |
| CP-10 | New Eden named-system scene | Accepted | System `30005286` resolves to A01; exact installed-client star graphics drive the sun color interpolation, and 565 seeded sprite stars render from a separate checksummed manifest. | The distant `universe.red` object and a nonzero authentic secondary SH source remain omitted; visible celestial and sun-effect work is tracked by CP-14 through CP-16. |
| CP-11 | Trinity object SH generation, upload, and V5 consumption | Accepted | The scene owns a real `Tr2ShLightingManager`; the Astero bridge publishes all seven packed coefficients and high-tier V5 consumes all seven. Primary and secondary intensities match the client value `3.14`. Exact New Eden sun/planet sources correctly remain below Trinity's cutoff. | A nonzero authentic source from another named system would broaden coverage but is not required for New Eden correctness. |
| CP-12 | Trinity tiled object lights | Accepted as capability | High-tier `quadv5`/`quadheatv5` reflect `LightBuffer` and `LightIndexBuffer`. Local-off, six-light authored, and one-light validation captures are distinct after `ComputeLightLists` resolves an 80x60 grid. The client effects, black fallback, and SoE faction banner remain checksummed build-tree assets. | The faction banner substitutes for player-specific photo-service logos; local-light shadows and visible haze/banner geometry remain disabled. |
| CP-13 | EVE driver depth/normal products | Accepted | `DepthMap` and `NormalMap` are requested from `EveSpaceSceneRenderDriver`, visualized on GPU, and captured after 180 frames. Depth has a clean reverse-Z silhouette; authored versus flat normal maps show the expected high-frequency response difference. | The visualizer is sample-owned diagnostic machinery; AO and shadow consumers remain disabled until RC-08. |
| CP-14 | New Eden authored planet surface | Accepted | Current client FSD resolves planet `40334264` to preset graphic `4321`, height graphics `3843`/`3903`, and their exact Black/DDS resources. The probe applies the client's `NormalHeight1`, `NormalHeight2`, `Random=64`, and deterministic high-resolution cloud mutations before synchronous preparation. | Aurora and data-driven planet FX remain omitted. |
| CP-15 | New Eden atmosphere and cloud layers | Accepted | `planetring.gr2` is converted to CMF with its reconstructed binormal stream. Native `atmosphere` and `earthlikeclouds` effects render distinct additive/transparent controls; `Reports/NewEdenAtmosphereResources.json` records six Metal payloads and eight Sandstorm cloud textures. | The installed client exposes only an unused `DoPlanetPreprocessing` queue shell, so no scheduler-generated maps are emulated. |
| CP-16 | New Eden sun flare and god rays | Accepted | `SunFlares` uses converted `unitplane` geometry; `yellow_small.black` retains 11 ordered CMF mesh areas and two foreground plus two background native occluders; client-colored god rays execute before exposure and tone mapping. `Reports/NewEdenSunFxResources.json` records 43 resources. | Whisps, particle-corona branches, bloom, and volumetric/froxel effects remain deferred. |
| CP-17 | Native SOF visible-attachment machinery | Accepted | `EveSpriteSet`, `EveSpotlightSet`, and `EvePlaneSet` submit through `Tr2QuadRenderer`; `EveHazeSet` and `EveBannerSet` submit additive batches. Six isolated/all captures, a 540-frame orbit, depth/normal controls, and HDR/exposure runs pass. `Reports/AsteroAttachmentResources.json` records the complete staged graph. | Static identity bone deltas are used; boosters and trails are tracked by RC-05D. Damage FX and attachment shadows remain deferred. Indexed overlays are accepted under CP-20. |
| CP-18 | Native cascaded shadows and bent-normal AO | Accepted | The Astero is a real `IEveShadowCaster`; three accepted cascades commit two V5 batches each. Named `ShadowMap`, `CascadedShadowDepth`, and `SSAOMap` outputs remain nonuniform with ultra dynamic reflections enabled. | Cinematic placement disables exact-system planet eclipsing; current macOS client parity disables local-light shadows under RC-08B. |
| CP-19 | Native ultra reflection capture and filtering | Accepted | Client prefilter, main filter, mip generation, and cube-copy stages are synchronously validated and checked at dispatch. A sample-owned six-face HDR visualizer proves the 256x256 eight-mip result is nonblack and nonuniform after two warm-up frames. `Reports/AsteroV5IndirectResources.json` records 18 external inputs. | Exact-system mode remains the authority for celestial placement; cinematic mode is the material-composition gate. |
| CP-20 | Native indexed SOF decals | Accepted | `IEveSpaceObjectDecalOwner` retains authored set/item order, raw transforms, high-detail static index arrays, V5 decal materials, faction maps, and kill-count display data. All 11 renderables commit native decal batches; `Reports/AsteroDecalResources.json` records 37 external inputs. | The kill counter defaults to zero without live ship state; lower imported LOD geometry, dynamic projection, and decal shadow/depth products remain deferred. |
| CP-21 | Native raster dynamic-light shadow atlas writer | Accepted as capability | Validation mode selects one point light and six faces. Authored mode selects six point lights (two haze and four banner), renders 36 faces into a valid 16384x16384 D32 atlas, and commits two hull/booster batches per accepted caster pass. A sample-owned resolver also emits a full-resolution R16_UINT light mask. | The current macOS client defaults the feature off and all 2,224 shipped receiving candidates compile the binding `readnone`. Explicit authored eligibility remains the `probe-all-active` approximation for future diagnostics. |
| CP-22 | Direct complete FP16 composition observability | Accepted | `PreTonemapColor` retains full-resolution `R16G16B16A16_FLOAT` before exposure/tone mapping. Raw validation, four distinct controls, complete inventory checks, all named products, and windowed/native 540-frame pacing runs pass. | Reinhard-plus-gamma PNGs remain diagnostic only; CP-23 supplies the accepted client tone path. |
| CP-23 | Client histogram exposure and baked Uncharted2 output | Accepted | `PostTonemapColor`, the 65-value histogram, and the eight-float exposure state are captured from one frame. The CPU reconstruction differs by at most one 8-bit code value; both temporal rates and matched exposure-off/client dynamic-range gates pass. | The current client container bakes Uncharted2 and exposes no runtime tone-method permutation. ACES fields remain serialized but inactive. |
| CP-24 | Client legacy bloom and film grain | Accepted | `BloomMap` retains the half-resolution FP16 high-pass/blur result and `FinalPostProcessColor` retains the post-grain output. Exact Black values and installed-client `newBloom=false` selection are validated; all native passes and isolated A/B captures pass. | LUTs, color correction, distortion, volumetrics, and the unused six-level new-bloom branch remain outside RC-11. |
| CP-25 | Native authored distortion composition | Accepted | Astero group 0 submits 120 indices through `TRIBATCHTYPE_DISTORTION`; `DistortionMap` reports 124,290 non-neutral pixels in localized bounds at the integrated gate. Scene-color copy and one foreground compositor draw succeed, and a frozen matched validator proves distinct pre-tone and final output. | Cloaking, warp tunnels, camera distortion, particles, and volumetric/froxel effects remain outside this checkpoint. |

## Rung-model holes

| Original claim | Missing contract | Correction |
| --- | --- | --- |
| Rung 3: model through EVE scene | Representative scene/background, complete object construction, SH/local lights, and effect areas | Split into 3A geometry, 3B object/material, 3C in-space scene, and 3D object lighting. |
| Rung 3E: visible SOF attachments | Authored additive and quad geometry was absent from the light-only bridge | Require independent family controls and native lifecycle submission before HDR composition. |
| Rung 3F: indexed SOF decals | Authored projected markings, faction logos, and kill state were absent | Require native indexed overlay batches and prove that all non-color products remain unchanged. |
| Rung 4: HDR/postprocess | Complete HDR composition before exposure; representative background luminance | RC-09 accepts FP16 composition, RC-10 accepts client exposure/tone output, and RC-11 accepts the client-selected legacy bloom and film-grain finish. |
| Rung 5: depth/normal after post | Depth/normal are prerequisites for AO, shadows, and complete composition | Validate products before accepting postprocess fidelity. |
| Rung 4B: shadows and AO | Native products require the client reflection/SH contract to compose correctly | RC-08A restores ultra dynamic reflections and client SH intensity before accepting RC-08; keep off/static controls and CP-18 products as regressions. |
| Rung 4C: local-light shadows | A valid atlas and caster pass do not prove material consumption | Current macOS client parity disables the feature and ships no receiving material. Keep CP-21 explicit diagnostics, but do not block HDR composition on an unavailable reference-client capability. |
| Rung 6B: authored distortion | Retained geometry and textures did not prove map generation or scene warping | RC-12A requires the native vector map, FP16 scene copy, compositor draw, matched off/on hashes, and byte-identical non-color products. |
| Rung 8: background as an optional effect | Background drives reflections and exposure statistics | Move background to RC-03, before lighting and postprocess acceptance. |

## Active work queue

1. Isolate volumetric/froxel effects under RC-12B.
2. Reconstruct authored booster plumes and trails under RC-05D.
3. Validate velocity and TAA under RC-13 with engine effects present.

RC-12A distortion is accepted. Volumetrics remain active under RC-12B;
booster/engine effects and velocity/TAA remain queued behind that unit.

## Evidence policy

Every accepted item must preserve:

- the exact command and finite frame count;
- status 0 and no Granny linkage;
- a still after any required temporal warm-up;
- an A/B image against the preceding accepted direct-path state;
- a resource manifest or log proving authored inputs were found;
- explicit notes for approximated, omitted, or synthetic inputs.

Artifacts remain under the sample build output and are not source-controlled.
This file tracks their purpose and acceptance state; the bring-up journal
records the implementation history and architectural discoveries.
