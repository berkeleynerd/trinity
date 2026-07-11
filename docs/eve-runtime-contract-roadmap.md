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
| RC-05C | Render indexed SOF decal sets | Blocked | RC-05B | Indexed decals retain authored ordering, materials, transforms, and depth behavior without corrupting accepted hull or attachment products. |
| RC-06 | Supply object SH and local/secondary lighting | Accepted | RC-04, RC-05 | The probe uses the client's high `SM_3_0_DEPTH` tier. Exact New Eden sun/planet inputs correctly produce zero secondary SH after Trinity's cutoff; the synthetic control proves the same upload path. Six authored `primary`/`soe` haze/banner lights resolve and rotate with the ship, and local off/authored/validation captures are distinct. |
| RC-07 | Validate depth and normal products | Accepted | RC-05 | The driver publishes reverse-Z `D32_FLOAT` depth and `R10G10B10A2_UNORM` normals through named outputs. A sample-owned GPU visualizer produces coherent 180-frame Astero captures; authored and flat-normal controls preserve silhouette/camera while differing in expected surface detail. |
| RC-08 | Add shadows and AO | Blocked | RC-06, RC-07 | Shadow-caster batches and AO consume validated products; stills show plausible contact/shape cues without geometry or lighting regression. |
| RC-09 | Accept complete HDR scene composition | Blocked | RC-03, RC-04, RC-04B, RC-04C, RC-05B, RC-06, RC-08 | Model, background, visible celestials and sun effects, visible attachments, direct/indirect lighting, reflection, depth/normal, shadows, and AO coexist in FP16 with stable frame pacing. |
| RC-10 | Reaccept exposure and tone mapping | Blocked | RC-09 | Settled `hdr-exposure` captures use the complete scene histogram and improve dynamic range without clipping, pumping, or hiding material errors. |
| RC-11 | Add bloom and film grain | Blocked | RC-10 | Bloom is accepted independently before film grain; each has an A/B capture and clean finite run. |
| RC-12 | Add distortion and volumetrics | Blocked | RC-09 | Each effect has its authored geometry/resources and a separate visual gate. |
| RC-13 | Add velocity and TAA | Blocked | RC-05, RC-07 | Current/previous camera and object transforms produce valid velocity; TAA is accepted only after direct velocity visualization. |

## Capability evidence tracker

These checkpoints prove machinery, not necessarily visual fidelity.

| ID | Capability | Status | Evidence | Fidelity limitation |
| --- | --- | --- | --- | --- |
| CP-00 | TrinityAL Metal presentation | Accepted | Pyramid and finite-frame smoke runs. | Does not exercise Trinity scene resources. |
| CP-01 | Public glTF-to-CMF animation path | Accepted | Box and Fox model/animation captures. | Standalone CPU skinning and sample materials. |
| CP-02 | EVE driver shell and empty scene | Accepted | `shell` and `scene` runs return zero. | No scene content. |
| CP-03 | SharedCache Astero conversion | Accepted | All three GR2 groups retain independent index ranges; isolated hull, booster, and distortion-geometry captures are coherent. | Build-time conversion bypasses runtime GR2 loading. |
| CP-04 | Native EVE render batch | Accepted | `Main` and `Depth` techniques submit through `ITr2Renderable`. | Bridge is not a complete SOF-built `EveSpaceObject2`. |
| CP-05 | Authored V5 material | Accepted as capability | Client `quadv5` and `quadheatv5`, SOF material/heat constants, logical textures, and independent batches load successfully. | SH/local lighting remains incomplete; distortion composition is separately gated. |
| CP-06 | FP16 resolve, tone mapping, and blit | Accepted as capability | `hdr-blit` and `hdr-post` complete finite captures. | Input scene composition is incomplete. |
| CP-07 | Client dynamic exposure compute | Accepted as capability | `hdr-exposure` runs histogram, merge, measure, and tone passes for 180 frames. | Histogram currently sees a mostly black synthetic composition. |
| CP-08 | Fitting-scene illumination transport | Accepted as capability | Authored fitting sun, ambient, reflection intensity, and cube reach per-frame V5 constants. | Fitting is not representative in-space lighting and supplies no secondary SH sources. |
| CP-09 | Authored universe background | Accepted | A01 background effect, cube textures, star map, reflection cube, and low-quality resources load from a checksummed build manifest and survive model/HDR/exposure captures. | The fixture does not reconstruct a named system's sun object or seeded `EveStarfield`. |
| CP-10 | New Eden named-system scene | Accepted | System `30005286` resolves to A01; exact installed-client star graphics drive the sun color interpolation, and 565 seeded sprite stars render from a separate checksummed manifest. | The distant `universe.red` object and a nonzero authentic secondary SH source remain omitted; visible celestial and sun-effect work is tracked by CP-14 through CP-16. |
| CP-11 | Trinity object SH generation, upload, and V5 consumption | Accepted | The scene owns a real `Tr2ShLightingManager`; the Astero bridge publishes all seven packed coefficients. The exact New Eden sun and planet both fall below Trinity's contribution cutoff, while the `physicalMax=1.195372e+00` stress source produces coherent hull fill. | A nonzero authentic source from another named system would broaden coverage but is not required for New Eden correctness. |
| CP-12 | Trinity tiled object lights | Accepted as capability | High-tier `quadv5`/`quadheatv5` reflect `LightBuffer` and `LightIndexBuffer`. Local-off, six-light authored, and one-light validation captures are distinct after `ComputeLightLists` resolves an 80x60 grid. The client effects, black fallback, and SoE faction banner remain checksummed build-tree assets. | The faction banner substitutes for player-specific photo-service logos; local-light shadows and visible haze/banner geometry remain disabled. |
| CP-13 | EVE driver depth/normal products | Accepted | `DepthMap` and `NormalMap` are requested from `EveSpaceSceneRenderDriver`, visualized on GPU, and captured after 180 frames. Depth has a clean reverse-Z silhouette; authored versus flat normal maps show the expected high-frequency response difference. | The visualizer is sample-owned diagnostic machinery; AO and shadow consumers remain disabled until RC-08. |
| CP-14 | New Eden authored planet surface | Accepted | Current client FSD resolves planet `40334264` to preset graphic `4321`, height graphics `3843`/`3903`, and their exact Black/DDS resources. The probe applies the client's `NormalHeight1`, `NormalHeight2`, `Random=64`, and deterministic high-resolution cloud mutations before synchronous preparation. | Aurora and data-driven planet FX remain omitted. |
| CP-15 | New Eden atmosphere and cloud layers | Accepted | `planetring.gr2` is converted to CMF with its reconstructed binormal stream. Native `atmosphere` and `earthlikeclouds` effects render distinct additive/transparent controls; `Reports/NewEdenAtmosphereResources.json` records six Metal payloads and eight Sandstorm cloud textures. | The installed client exposes only an unused `DoPlanetPreprocessing` queue shell, so no scheduler-generated maps are emulated. |
| CP-16 | New Eden sun flare and god rays | Accepted | `SunFlares` uses converted `unitplane` geometry; `yellow_small.black` retains 11 ordered CMF mesh areas and two foreground plus two background native occluders; client-colored god rays execute before exposure and tone mapping. `Reports/NewEdenSunFxResources.json` records 43 resources. | Whisps, particle-corona branches, bloom, and volumetric/froxel effects remain deferred. |
| CP-17 | Native SOF visible-attachment machinery | Accepted | `EveSpriteSet`, `EveSpotlightSet`, and `EvePlaneSet` submit through `Tr2QuadRenderer`; `EveHazeSet` and `EveBannerSet` submit additive batches. Six isolated/all captures, a 540-frame orbit, depth/normal controls, and HDR/exposure runs pass. `Reports/AsteroAttachmentResources.json` records the complete staged graph. | Static identity bone deltas are used; boosters, trails, damage FX, indexed decals, distortion, and attachment shadows remain deferred. |

## Rung-model holes

| Original claim | Missing contract | Correction |
| --- | --- | --- |
| Rung 3: model through EVE scene | Representative scene/background, complete object construction, SH/local lights, and effect areas | Split into 3A geometry, 3B object/material, 3C in-space scene, and 3D object lighting. |
| Rung 3E: visible SOF attachments | Authored additive and quad geometry was absent from the light-only bridge | Require independent family controls and native lifecycle submission before HDR composition. |
| Rung 4: HDR/postprocess | Complete HDR composition before exposure; representative background luminance | Treat `hdr-post` and `hdr-exposure` as capability checkpoints until RC-09. |
| Rung 5: depth/normal after post | Depth/normal are prerequisites for AO, shadows, and complete composition | Validate products before accepting postprocess fidelity. |
| Rung 6: AO/shadows as optional quality | They materially affect shape readability and scene composition | Make them a direct-path prerequisite for RC-09. |
| Rung 8: background as an optional effect | Background drives reflections and exposure statistics | Move background to RC-03, before lighting and postprocess acceptance. |

## Active work queue

1. Add shadow-caster batches and enable AO one at a time under RC-08, preserving
   the accepted RC-07 product captures as controls.
2. Reconstruct indexed decal sets separately under RC-05C without enabling
   distortion or attachment shadows.

Bloom, film grain, distortion, volumetrics, velocity, and TAA are not in the
active queue. Their capability work remains useful, but they may not advance
the fidelity status until their direct-path prerequisites are accepted.

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
