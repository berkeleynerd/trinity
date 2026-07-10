# EVE Runtime-Contract Roadmap

This tracker separates two questions that the original quality ladder combined:

1. Does an individual Trinity/EVE capability execute on macOS Metal?
2. Does the composed renderer satisfy enough of the EVE runtime contract for a
   visually meaningful fidelity comparison?

Capability evidence is retained even when a later fidelity gate is incomplete.
No postprocess checkpoint can compensate for missing scene, object-lighting, or
render-product inputs.

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
| RC-05 | Complete the ship object/material-area contract | Accepted | RC-02 | Groups 0/1/2 remain distinct CMF sections. Hull and booster render through authored `quadv5`/`quadheatv5` batches; distortion geometry/effect/maps are validated and explicitly deferred to RC-12. `AsteroClientAssets.md` classifies every per-object default and absent area. |
| RC-06 | Supply object SH and local/secondary lighting | Partial | RC-04, RC-05 | The bridge participates in `ITr2ShLightingReceiver`, and direct-only, SH-only, and combined modes are independently observable. New Eden's one real planet is below Trinity's contribution cutoff; a stress source proves coefficient generation but byte-identical combined captures show that the selected opaque V5 passes do not consume the physical coefficient payload. |
| RC-07 | Validate depth and normal products | Partial | RC-05 | Direct captures prove depth range, handedness, normal encoding, tangent-space orientation, and background behavior. |
| RC-08 | Add shadows and AO | Blocked | RC-06, RC-07 | Shadow-caster batches and AO consume validated products; stills show plausible contact/shape cues without geometry or lighting regression. |
| RC-09 | Accept complete HDR scene composition | Blocked | RC-03, RC-04, RC-06, RC-08 | Model, background, direct/indirect lighting, reflection, depth/normal, shadows, and AO coexist in FP16 with stable frame pacing. |
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
| CP-10 | New Eden named-system scene | Accepted | System `30005286` resolves to A01; exact installed-client star graphics drive the sun color interpolation, and 565 seeded sprite stars render from a separate checksummed manifest. | The visible sun model, lens flare/god rays, distant `universe.red` object, SH, and local lights remain omitted. |
| CP-11 | Trinity object SH generation and upload | Accepted as capability | The scene owns a real `Tr2ShLightingManager`; the Astero bridge implements `ITr2ShLightingReceiver`, publishes all seven packed coefficients, and exposes direct/SH/combined captures. A labeled stress sphere produces `physicalMax=1.195372e+00`. | New Eden planet `40334264` is culled as negligible. Authentic-zero and stress-source combined PNGs are byte-identical, proving the selected opaque V5 passes do not use the physical coefficients; end-to-end SH consumption is not accepted. |

## Rung-model holes

| Original claim | Missing contract | Correction |
| --- | --- | --- |
| Rung 3: model through EVE scene | Representative scene/background, complete object construction, SH/local lights, and effect areas | Split into 3A geometry, 3B object/material, 3C in-space scene, and 3D object lighting. |
| Rung 4: HDR/postprocess | Complete HDR composition before exposure; representative background luminance | Treat `hdr-post` and `hdr-exposure` as capability checkpoints until RC-09. |
| Rung 5: depth/normal after post | Depth/normal are prerequisites for AO, shadows, and complete composition | Validate products before accepting postprocess fidelity. |
| Rung 6: AO/shadows as optional quality | They materially affect shape readability and scene composition | Make them a direct-path prerequisite for RC-09. |
| Rung 8: background as an optional effect | Background drives reflections and exposure statistics | Move background to RC-03, before lighting and postprocess acceptance. |

## Active work queue

1. Reconstruct the client object/local-light and area-selection inputs that
   accompany a SOF ship, including which materials actually request SH versus
   direct lighting.
2. Validate those inputs independently against the accepted direct sun and the
   zero-contribution New Eden planet control.
3. Resume depth/normal product validation only after the composed scene remains
   stable.

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
