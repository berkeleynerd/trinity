# macOS Metal Bring-up Journal

This document records the Apple Silicon macOS bring-up performed on the
`codex/trinityal-open-model-demo` branch. It is both a rebuild guide and an
architecture report: what works, what was disabled, what was approximated, and
where the next quality rung is blocked.

The important qualification is that this is an experimental standalone path.
It proves progressively larger parts of Trinity, from TrinityAL through the EVE
scene render driver, but it is not yet a complete EVE client resource stack.

## Current status

The following paths build and run on this host:

| Path | Target | Result |
| --- | --- | --- |
| Low-level Metal sample | `TrinityALHelloTriangle_metal` | A responsive Cocoa/Metal window rendering a rotating pyramid. The target kept its original hello-triangle name. |
| glTF conversion | `TrinityGltfToCmf` | Converts the supported glTF subset to CMF at build time. |
| Standalone CMF renderer | `TrinityALAnimatedModel_metal` | Renders the synthetic box and the animated Khronos Fox through TrinityAL. |
| EVE driver shell | `TrinityALEveSceneProbe_metal --quality-rung shell` | Creates a Trinity device and clears/presents through the low-level context. |
| EVE scene fixture | `TrinityALEveSceneProbe_metal --quality-rung scene` | Runs the real driver with the authored A01 universe by default; `--scene-fixture empty` preserves the empty control. |
| EVE batch bridge | `TrinityALEveSceneProbe_metal --quality-rung model` | Mechanically working: resolves the Astero, converts its opaque hull to CMF, inserts a Blue-managed renderable into an EVE scene, and submits a real `Tr2Effect` batch. It is not a complete `EveSpaceObject2` runtime contract. |
| EVE HDR/postprocess | `TrinityALEveSceneProbe_metal --quality-rung hdr-post` | Capability checkpoint: renders through the driver's FP16 target, resolves, executes the extracted client Metal tone-mapping container, and presents through client `Blit`. The scene composition is incomplete. |
| EVE dynamic exposure | `TrinityALEveSceneProbe_metal --quality-rung hdr-exposure` | Capability checkpoint: executes client histogram, histogram-merge, exposure-measure, and Uncharted 2 tone passes. It is not a fidelity milestone until a representative in-space background and lighting composition feed the histogram. |

The accepted EVE model rung now defaults to the Sisters of EVE Astero geometry
and its 1024x1024 base-color, normal, roughness, material, and glow maps from the
local EVE SharedCache. A sample-owned `.fx` file is compiled by Trinity's
`ShaderCompiler`, loaded by `Tr2EffectRes`, and submitted as an ordinary opaque
`Tr2RenderBatch`. This validates the native effect/material binding and EVE
depth/main pass machinery without claiming fidelity to the unavailable authored
complete EVE object, in-space scene, environment-lighting, or postprocess
runtime contract.

Rung 4 reads the client's macOS-specific resource index and copies selected
version-15 Metal effect containers into the build tree. The extracted
`ToneMapping.sm_hi`, `Blit.sm_hi`, and `BlitFiltered.sm_hi` all load and
execute through this checkout's `Tr2EffectRes` path.

The main implementation entry points are:

- [`samples/hello_triangle`](../samples/hello_triangle) for the first TrinityAL
  Metal presentation loop;
- [`tools/gltf_to_cmf`](../tools/gltf_to_cmf) for the importer library and CLI;
- [`samples/animated_model`](../samples/animated_model) for standalone CMF
  animation and texture experiments;
- [`samples/eve_scene_probe`](../samples/eve_scene_probe) for the full-screen
  quality ladder host;
- [`eve-runtime-contract-roadmap.md`](eve-runtime-contract-roadmap.md) for the
  authoritative direct-path dependencies and capability evidence tracker;
- [`TrinityStandaloneProbe.cpp`](../trinity/TrinityStandaloneProbe.cpp) for the
  opaque startup API and EVE object/renderable bridge;
- [`TrinityStandaloneCmfModel.cpp`](../trinity/TrinityStandaloneCmfModel.cpp)
  for CMF validation, texture-sidecar loading, rest-pose skinning, tangent
  generation, framing, and indexed static GPU data;
- [`EveSpaceSceneRenderDriver`](../trinity/Eve/EveSpaceSceneRenderDriver.cpp) and
  [`Tr2RenderBatch`](../trinity/TriRenderBatch.cpp) for the driver and custom
  batch integration points.

## Host snapshot

The last verified host configuration was:

| Component | Version |
| --- | --- |
| macOS | 26.5.2 (25F84) |
| Architecture | arm64 |
| Xcode | 26.6 (17F113) |
| CMake | 4.3.4 |
| Ninja | 1.13.2 |
| Git | 2.55.0 |
| Homebrew | 6.0.9 |
| Host Python | 3.14.6 |
| vcpkg tool Python | 3.12.9 |

`xcrun metal` resolves into the Xcode-managed Metal toolchain. Do not hard-code
that resolved path; use `xcrun metal` as the sample CMake rules do.

The relevant pinned submodules were:

```text
carbonengine/vcpkg-registry  8bd572152d56d5d4b26be0e129615c1c43df438e
microsoft/vcpkg              031f4afe03be385aee354f15be6e6b1fe57497c7
```

## Rebuild from a clean checkout

Install the host build tools. Xcode command-line tools must already be usable.

```sh
brew install cmake ninja
```

Initialize and bootstrap the two pinned dependency repositories:

```sh
git submodule update --init --recursive \
  vendor/github.com/microsoft/vcpkg \
  vendor/github.com/carbonengine/vcpkg-registry
./vendor/github.com/microsoft/vcpkg/bootstrap-vcpkg.sh
```

Configure the Apple Silicon debug build:

```sh
mkdir -p regcache buildtrees

export PATH_TO_VCPKG_ROOT="$PWD/vendor/github.com/microsoft/vcpkg"
export X_VCPKG_REGISTRIES_CACHE="$PWD/regcache"
export VCPKG_BINARY_SOURCES=clear

CMAKE_GENERATOR="Ninja Multi-Config" cmake --preset arm64-osx-debug \
  -DBUILD_METAL=ON \
  -DBUILD_SHADER_COMPILER=ON \
  -DBUILD_TESTING=OFF \
  -DWITH_GRANNY=OFF \
  -DBUILD_GLTF_IMPORTER=ON \
  -DBUILD_ANIMATED_MODEL_SAMPLE=ON \
  -DBUILD_EVE_SCENE_PROBE=ON \
  -DBUILD_HELLO_TRIANGLE=ON \
  -DVCPKG_INSTALL_OPTIONS="--x-buildtrees-root=$PWD/buildtrees"
```

The EVE probe defaults to the current macOS launcher cache at
`~/Library/Application Support/EVE Online/SharedCache`. Override it for a
different installation with
`-DEVE_SCENE_PROBE_SHARED_CACHE=/absolute/path/to/SharedCache`. Only selected
resources and generated intermediates are copied into the build tree; the
cache itself remains external.

The preset supplies `PATH_TO_VCPKG_ROOT`, but exporting it explicitly is useful
for command-line regeneration and IDE-launched CMake processes. Without it, one
failed regeneration tried to include `/scripts/toolchains/osx.cmake`.

Build the focused experimental targets:

```sh
cmake --build .cmake-build-arm64-osx-debug --config Debug \
  --target TrinityGltfToCmf \
           TrinityALHelloTriangle_metal \
           TrinityALAnimatedModel_metal \
           TrinityALEveSceneProbe_metal
```

The full root build also succeeds with this configuration:

```sh
cmake --build .cmake-build-arm64-osx-debug --config Debug --target all
```

For CLion, select the `arm64-osx-debug` CMake preset and set the same cache
options. Ensure `PATH_TO_VCPKG_ROOT` is present in the CMake profile environment.

## Running the samples

The CMake run targets arrange working directories and runtime assets:

```sh
cmake --build .cmake-build-arm64-osx-debug --config Debug \
  --target run_TrinityALHelloTriangle_metal

cmake --build .cmake-build-arm64-osx-debug --config Debug \
  --target run_TrinityALAnimatedModel_metal

cmake --build .cmake-build-arm64-osx-debug --config Debug \
  --target run_TrinityALAnimatedModelBox_metal

cmake --build .cmake-build-arm64-osx-debug --config Debug \
  --target run_TrinityALEveSceneProbe_metal
```

The EVE probe is full-screen by default. Use a window and a finite frame count
for smoke testing:

```sh
./.cmake-build-arm64-osx-debug/samples/eve_scene_probe/Debug/TrinityALEveSceneProbe_metal_debug \
  --windowed 640x480 \
  --frames 180 \
  --quality-rung model \
  --asset astero \
  --material-mode probe \
  --scene-fixture new-eden
```

Capture a comparison still and metadata:

```sh
./.cmake-build-arm64-osx-debug/samples/eve_scene_probe/Debug/TrinityALEveSceneProbe_metal_debug \
  --windowed 640x480 \
  --frames 180 \
  --quality-rung model \
  --asset astero \
  --material-mode probe \
  --capture-prefix .cmake-build-arm64-osx-debug/samples/eve_scene_probe/captures/rung3
```

This writes `rung3_astero_model_probe.png` and `rung3_astero_model_probe.txt`. On a Retina
display, a 640 by 480 point window normally renders at 1280 by 960 backing
pixels. The captured window image also contains the title bar.

The probe accepts `--scene-fixture empty|fitting|a01|new-eden` and defaults to
`new-eden`. `empty` preserves the original black-clear control, `fitting`
selects the client fitting-room fixture, and `a01` preserves the raw universe
Black with its default sun values.

Run the two rung-4 checkpoints independently:

```sh
./.cmake-build-arm64-osx-debug/samples/eve_scene_probe/Debug/TrinityALEveSceneProbe_metal_debug \
  --windowed 640x480 --frames 180 --asset astero --quality-rung hdr-blit

./.cmake-build-arm64-osx-debug/samples/eve_scene_probe/Debug/TrinityALEveSceneProbe_metal_debug \
  --windowed 640x480 --frames 180 --asset astero --quality-rung hdr-post \
  --material-mode eve-v5 \
  --capture-prefix .cmake-build-arm64-osx-debug/samples/eve_scene_probe/captures/rung4
```

Inspect individual material inputs without changing the native batch path:

```sh
./.cmake-build-arm64-osx-debug/samples/eve_scene_probe/Debug/TrinityALEveSceneProbe_metal_debug \
  --windowed 640x480 \
  --frames 60 \
  --asset astero \
  --material-view basecolor
```

Valid views are `lit`, `basecolor`, `normal`, `roughness`, `material`, `glow`,
`d`, `mask`, and `p3`. Non-lit captures include the view name in the PNG and
metadata filename.

## The path we took

### 1. Establish a Metal presentation loop

The first sample created an `NSApplication`, a centered 640 by 480 `NSWindow`,
and an `NSView` backed by `CAMetalLayer`. It passed the native view to
`Tr2PrimaryRenderContextAL`, created buffers and shaders through TrinityAL, and
presented a frame.

The initial triangle became a rotating pyramid so that transform, depth, face
ordering, and continuous presentation could be checked visually. The old target
name remains to preserve the original smoke-test entry point.

The sample shaders are compiled with `xcrun metal`. The small
`cmake/bin2h.py` helper turns the resulting shader binaries into C arrays so the
sample has no runtime shader-file dependency.

This milestone exercised TrinityAL and the Metal backend. It did not exercise
Blue, Trinity resource loading, Trinity effects, render batches, or EVE scenes.

### 2. Use CMF as the open runtime format

The repository already depended on CarbonMesh. Its CMF `Data` structure holds:

- meshes and LOD geometry;
- skeleton bone names, parent indices, rest transforms, and inverse bind
  transforms;
- animations made from named channels and interpolation curves.

That was the key to avoiding the licensed Granny SDK. Granny was not the only
place the runtime could represent skeletal animation: CMF and the existing CMF
animation helpers already provided the necessary data model and evaluation
primitives.

We added the `gltf-importer` vcpkg feature with `cgltf`, Google Draco, `stb`, and
`libwebp`, then built two layers:

- `TrinityGltfImporter`, a static conversion library;
- `TrinityGltfToCmf`, a command-line converter used by sample asset rules.

The CLI is:

```sh
TrinityGltfToCmf \
  --input model.glb \
  --output model.cmf \
  --summary
```

The current importer handles triangle primitives, one CMF LOD, positions,
normals, texture coordinates, material base-color factors, `JOINTS_0`,
`WEIGHTS_0`, 16- or 32-bit indices, glTF skins, and `STEP` or `LINEAR`
translation/rotation/scale animation channels. Node matrices must decompose and
recompose as TRS within tolerance. Draco-compressed static triangle meshes can
be decoded when they contain position, normal, and texture-coordinate data.

It explicitly rejects:

- non-triangle primitives;
- `EXT_meshopt_compression`;
- Draco-compressed skinned primitives;
- unsupported animation interpolation, including `CUBICSPLINE`;
- node matrices that cannot be represented by the supported TRS path.

For the standalone texture experiment, the converter decodes base-color images
to RGBA and writes a raw `.rgba` file plus `.txt` dimensions beside the CMF.
Importer v1 writes only the first distinct base-color texture sidecar and warns
when a model uses more than one. This is a sample transport, not a Trinity
material or texture resource format.

### 3. Validate geometry with a synthetic box

The first animated Fox output looked like independently moving planes. Before
changing animation math, we added a synthetic box glTF to isolate the stages:

```text
glTF accessor decoding -> CMF writing -> CMF reading -> vertex layout -> projection -> draw
```

The solid box rendered correctly. That proved the basic mesh and draw pipeline
and made it possible to debug model-specific topology, skinning, and transforms
without conflating them with Metal setup.

The box remains a useful deterministic regression asset because it has known
positions, winding, and bounds and requires no external license or download.

### 4. Animate and texture the Fox

The Khronos Fox asset is converted to CMF at build time. The standalone sample:

- selects `Run` when available, otherwise the first animation;
- evaluates `cmf::AnimationSequencer` into a `cmf::RestPose`;
- calls `cmf::ComputeWorldTransforms`;
- CPU-skins each vertex in the CMF order
  `inverseBindTransform * worldTransform`;
- projects vertices to clip space on the CPU;
- submits the resulting triangles through TrinityAL.

CPU skinning and CPU projection were intentional bring-up choices. They expose
the imported data and transform order directly and avoid making GPU skinning,
constant-buffer layouts, and the Trinity material system simultaneous unknowns.

The texture experiment also remained sample-local. The decoded base-color RGBA
sidecar is uploaded directly by the sample. The renderer applies simple custom
lighting, attempts 4x MSAA with fallback, and uses 16x anisotropic sampling. It
does not use EVE's material, lighting, environment, or postprocess systems.

### 5. Try an external ship model

The original `~/Downloads/11989_lite.glb` was a GitHub HTML file saved with a
`.glb` extension. Its page identified the actual source as
`EstamelGG/EVE_Model_Gallery/docs/models/11989_lite.glb`. We preserved the HTML
as `11989_lite.glb.github-page.html` and downloaded the raw 1.5 MiB GLB into the
expected Downloads path.

The real asset uses `KHR_draco_mesh_compression` and `EXT_texture_webp`. It has
one static mesh, 6,564 uploaded vertices, 23,835 triangle indices, and one
1024x1024 base-color texture. Adding Google Draco to the importer made the
normal build-time path work:

```text
11989_lite.glb -> Draco decode -> CMF + base-color sidecar -> EVE renderable
```

The source also contains normal and metallic-roughness textures. Rung 3 does
not import those yet; it combines the base-color texture with imported vertex
normals and sample-owned directional lighting.

The source repository has a README but no explicit license file was found, so
the ship remains a host-local input and is not committed to this repository.
The CMake asset rule consumes `~/Downloads/11989_lite.glb`, generates the CMF
and texture sidecars under the build tree, and copies them beside the sample.

### 6. Resolve and convert the installed Astero

The probe now reads the modern EVE launcher index from:

```text
~/Library/Application Support/EVE Online/SharedCache/
  ResFiles/
  tq/EVE.app/Contents/Resources/build/resfileindex.txt
```

`tools/shared_cache_to_gltf` resolves logical `res:/` paths through that index
without reconstructing or copying the 49 GB cache. For the Astero it selects:

```text
res:/dx9/model/ship/soe/frigate/soef1/soef1_t1.gr2
res:/dx9/model/ship/soe/frigate/soef1/soef1_t1_a.dds
```

The build-time model path is:

```text
SharedCache GR2 -> @carbonenginejs/format-gr2 -> glTF + BIN
                -> TrinityGltfToCmf -> Astero.cmf
```

This is not Granny emulation inside Trinity. The open GR2 reader is a pinned
npm build dependency used before runtime; Trinity receives only CMF. The
selected LOD contains 7,194 source vertices and three index groups. Groups 1
and 2 contribute 7,326 opaque hull triangles. Group 0 is a 40-triangle
fan-shaped effect surface that becomes an incorrect filled octagon under the
probe's one-opaque-material approximation, so the Astero CMake rule explicitly
excludes it. The open split ring visible in the reference model is part of the
retained hull geometry.

The full albedo is a 1024x1024 DX10 BC7 DDS. CarbonImageIO recognizes the file
but cannot convert BC7 to RGBA8 in this macOS package, so `TrinityDdsToRgba`
uses CarbonImageIO for supported DDS formats and a pinned MIT-licensed `bcdec`
fallback for BC7. The decoded RGBA and dimensions are written beside the CMF
for the sample-owned texture upload. The same tool now decodes DXT1/BC1 and
legacy ATI1/BC4 and ATI2/BC5 resources.

These are the full-detail resources, not the tiny `_lowdetail.dds` variants,
and the probe does not downsample them. Base color, normal, roughness,
material, glow, dirt, and paint mask are 1024x1024. The authored distortion
layer map is 512x512.

The cached `res:/dx9/model/spaceobjectfactory/hulls/soef1_t1.black` metadata
identifies the opaque `quad/quadv5.fx` slots: `_a` is `AlbedoMap`, `_d` is
`DirtMap`, `_g` is `GlowMap`, `_m` is `MaterialMap`, `_n` is `NormalMap`,
`_p3` is `PaintMaskMap`, and `_r` is `RoughnessMap`. The separate `_mask`
resource is `Layer2Map` for the omitted distortion area, not an opaque hull
input. The probe therefore applies conservative dirt darkening and a fixed SOE
red through the paint mask, while exposing `_mask` only as a debug view.

### 7. Inspect authored SOF and Metal effects

The current client Black files are runtime-compatible with this checkout's
typed `EveSOFData*` registrations. The probe includes a build-only inspection
target that copies the required files from SharedCache, decodes them with
Carbon `BlackReader`, loads the authored Metal effect with `Tr2EffectRes`, and
writes a report:

```sh
cmake --build .cmake-build-arm64-osx-debug --config Debug \
  --target inspect_TrinityEveAsteroAssets
```

The report is written to
`.cmake-build-arm64-osx-debug/samples/eve_scene_probe/Reports/AsteroClientAssets.md`.
No client assets are copied into the source tree.

The Astero metadata establishes the authored area split:

- group 1 is `area_hull`, using `quad/quadv5.fx`;
- group 2 is `area_booster`, using `quad/quadheatv5.fx` and four heat parameter sets;
- group 0 is `area_fx_distortion`, using `fx/fxdistortionv5.fx` with the `_mask` layer;
- the hull contains no separate transparent or additive area;
- ten decal sets, four sprite sets, two spotlight sets, four plane sets, and one light set are separate attachments.

The SOE base faction assigns the primary material channels to
`chrome_metallic`, `grey_steel_brushed`, `white_ghost_matt`, and
`red_crimson_enamel`. Glass is channel four of the `Glass` area type and uses
`glass_soe`. The material Black files provide exact diffuse, dust, Fresnel, and
gloss values, replacing the need to infer those values from screenshots.

The current `quadv5.sm_hi` is also compatible with Trinity's effect container
reader. It is format version 15, has seven permutation dimensions, and exposes
`Main`, `Depth`, `Picking`, `Shadow`, `RtShadow`, and `DynamicLightShadow`
techniques. Its default vertex contract requires `POSITION0`, `TANGENT0`,
`TEXCOORD0`, `TEXCOORD1`, and `BLENDINDICES0`. The authored A/B path now
transports imported joint indices and CMF `PackedTangent` values. Metal supplies
a constant zero stream for the absent second UV set, matching this Astero source,
which has no second UV channel.

Use `--material-mode probe` for the compact comparison shader or
`--material-mode eve-v5` for the extracted client `quadv5` shader. The V5 mode
loads the exact SOF hull, faction, and four primary material Black files, binds
their seven logical DDS resources, uses the SOE hull glow color, and stages the
client reflection cube under its original `res:/` path.

The standalone host cannot use `BlueResMan::LoadObject` directly because that
entry point assumes the full client Stackless scheduler and `PyOS` bootstrap.
The inspector instead creates Carbon's registered `BlackReader`, uses its
non-yielding stream API, and keeps object initialization disabled. This is a
host-lifecycle limitation, not a Black format incompatibility.

### 8. Enter the Trinity/EVE scene stack

The EVE probe links `trinity_metal`, not only `TrinityAL_metal`. It initializes
Blue and Trinity, creates an `EveSpaceScene`, supplies `TriView` and
`TriProjection`, and calls `EveSpaceSceneRenderDriver::Validate` and `Execute`.

`trinity_metal` hides most C++ symbols. The sample therefore uses a small,
exported opaque C ABI in `TrinityStandaloneProbe.cpp`. That translation unit
lives inside Trinity and can work with Blue objects and private C++ types while
the Objective-C++ executable sees only a stable handle and functions.

The probe's startup order is:

1. start the Blue module;
2. initialize valid `key=path` Blue search paths such as `root=`, `bin=`, and
   `res=`;
3. initialize Blue resource loading;
4. run the exported Trinity class-registration startup;
5. create the Blue-managed `TriDevice`, view, projection, scene, driver, and
   renderable.

The bridge object implements both `IEveSpaceObject2` and `ITr2Renderable`.
It is inserted into `EveSpaceScene::Objects()`, participates in synchronous EVE
updates, and contributes an ordinary opaque `Tr2RenderBatch` during scene batch
collection. No custom-draw extension to the shared batch API is required.

For the model path, the bridge validates CMF, preserves indexed source vertices,
generates tangents, uploads static vertex/index buffers and eight texture
resources, then binds them through `Tr2Effect`. Rotation and projection happen
in the effect's vertex shader. Skinned CMF inputs such as Fox are evaluated once
in their CMF rest pose so they remain useful static regressions in this probe;
animated playback stays in `TrinityALAnimatedModel_metal`.

The probe effect contains `Main` and `Depth` techniques. `Main` applies the
sample approximation of material lighting. `Depth` writes an encoded normal
while the EVE driver owns its depth attachment. With `forceNormalMap` enabled,
the model participates in the driver's real depth/normal prepass before its
main opaque pass. Missing or invalid CMF input, missing effect resources, or
missing techniques now fail the rung instead of silently producing a blank
frame.

## Architecture discovered

The practical layering found during bring-up is:

```text
Cocoa / CAMetalLayer
        |
        v
TrinityAL Metal backend
  contexts, buffers, textures, shaders, draw, present
        |
        +---------------- standalone samples ----------------+
        |                                                     |
        v                                                     v
CarbonMesh / CMF                                      direct custom shaders
  mesh, skeleton, animation
        |
        v
Trinity + Blue
  object model, resources, effects, batches, device lifetime
        |
        v
EVE scene and EveSpaceSceneRenderDriver
  scene traversal, frame targets, depth/main passes,
  postprocess, AO, shadows, velocity/TAA, optional effects
```

Three conclusions follow from this layering.

First, TrinityAL is independently useful. A public model can be imported into
CMF, animated, and drawn without bringing up the full client resource graph.
That is how the Fox path bypasses Granny and most of Trinity while still using
real TrinityAL and CarbonMesh code.

Second, CMF is more than static mesh storage. It includes skeleton and animation
data, and the CarbonMesh runtime provides pose and world-transform helpers. The
remaining `Tr2Granny*` and `TriGranny*` names are compatibility-era API names;
their presence does not prove that the Granny SDK is linked for the CMF path.

Third, EVE's quality settings are prerequisites, not isolated switches. HDR
postprocessing needs effect resources. AO needs valid depth and normal
products. Cascaded shadows need shadow-caster batches. TAA needs meaningful
velocity data. Volumetrics and distortion need their scene resources and
techniques. Enabling a quality enum without producing those inputs cannot create
the intended result.

## What was disabled

| Item | Setting or behavior | Reason |
| --- | --- | --- |
| Granny SDK | `WITH_GRANNY=OFF` | Keep the runtime public and avoid the licensed dependency. Current EVE `.gr2` is converted by an open build-time tool; Trinity itself neither loads `.gr2` nor emulates Granny. |
| Repository tests | `BUILD_TESTING=OFF` | The initial macOS demo work focused on executable smoke tests; the existing Trinity tests are not wired as an Apple test executable. |
| Dynamic exposure | Isolated checkpoint only | Client histogram and exposure effects execute, but their visual result is not accepted against the incomplete black-background scene. |
| Bloom and film grain | Not enabled | They are intentionally blocked until a representative in-space scene makes HDR composition and exposure meaningful. |
| AO and shadows | Not enabled | The bridge now emits depth/normal-compatible batches, but AO has not been visually validated and shadow-caster techniques/resources are still absent. |
| TAA/velocity | Not enabled | The bridge has no meaningful velocity output. |
| Distortion, volumetrics, and complete reflection/background hooks | Not enabled | A fitting reflection cube is bound, but the broader systems require additional effects, resources, scene inputs, and compatible batches. |
| Full Python/BeOS runtime | Not started | A Python interpreter is initialized because Blue's synchronous resource helper assumes the GIL exists, but there is no Python task scheduler or full client host. Black inspection uses the non-yielding reader API instead of `BlueResMan::LoadObject`. |
| GPU skinning | Not implemented | CPU skinning kept import and transform debugging observable. |
| Full glTF material model | Not implemented | The sample handles one base-color sidecar, not Trinity materials or arbitrary glTF PBR graphs. |

`otool -L` was used on the sample and `_trinity_metal_debug.so`; neither linked
Granny. Compatibility-named source files may still compile when
`WITH_GRANNY=OFF`, but the proprietary library is not a runtime dependency.

## What was stubbed, faked, or bypassed

| Approximation | Exact behavior | Consequence |
| --- | --- | --- |
| EVE rung-3 probe model | The real ship CMF is loaded into static indexed GPU buffers and transformed in a sample-owned Trinity effect. | This remains the stable visual control selected by `--material-mode probe`. |
| Authored V5 material | `--material-mode eve-v5` uses the extracted client `quadv5`, exact SOF primary material constants, logical DDS resources, and reflection cube. | The hull material is authentic; booster heat, attachments, decals, client scene grading, and full SOF object construction remain outside the bridge. |
| Postprocess bypass | `Settings::bypassPostProcessing` renders scene color directly to the destination. | The rung-3 machinery checkpoint works without tone mapping and blit effects, but it is neither HDR/postprocessed output nor a complete scene composition. |
| Neutral tone-map configuration | The extracted EVE tone effect runs without scene-authored exposure, grading, bloom, grime, or LUT inputs. | The shader is authentic, but its output is not the complete EVE look-development configuration. |
| Standalone texture transport | The first base-color image is emitted as raw RGBA plus text dimensions. | It is easy to inspect but is not a production CMF texture/material package. |
| Lighting | The probe shader owns a small light model; V5 loads the client's fitting scene and uses its authored sun, ambient color, reflection intensity, and reflection cube. | This validates per-frame transport, not realistic in-space lighting. The background is black, the bridge publishes zero SH coefficients, and no local/secondary lights or representative exposure field exist. |
| Camera and per-object data | V5 consumes the driver's camera constants and modern `EveSpaceObjectVSData`/`EveSpaceObjectPSData`; the static bridge reports zero active bones. | Object transforms are native, but animated bone uploads and previous-frame velocity are not yet supplied. |
| Depth/normal participation | The effect supplies a `Depth` technique and the driver allocates its normal target. | The pass is native, but a direct debug capture of the normal product is still needed before AO is accepted. |
| External ship source | The GLB remains in `~/Downloads` and generated files remain under the build tree. | The sample is reproducible on this host without committing an asset whose repository has no explicit license. |
| Astero material groups | The build imports opaque groups 1 and 2 and omits effect group 0. | The hull and open ring match the source silhouette; metadata confirms the omitted surface belongs to `fxdistortionv5.fx`. |
| EVE texture set | `_a`, `_n`, `_r`, `_m`, `_g`, `_d`, and `_p3` bind to authored V5 names; `_mask` remains a probe-only distortion debug view. | The opaque hull input contract is satisfied, while the separate distortion area remains deferred. |
| SharedCache ownership | Logical resources are resolved in the external launcher cache and selected files are copied only into the build tree. | No 49 GB cache duplication or checked-in EVE asset payload is required. |

## Portability and lifecycle discoveries

Several failures were not rendering errors; they exposed assumptions normally
satisfied by the full client host.

- `TriDevice` is a Blue reference-counted object and cannot safely be a normal
  stack/value member. The probe creates it through `BluePtr<TriDevice>` and
  tears down scene resources, ticks, globals, and references in ownership order.
  This fixed a Blue `mLockCount` shutdown assertion.
- `TriDevice::Update` assumes a live `PyOS`. The standalone probe does not call
  it. `EveSpaceSceneRenderDriver::Execute` receives explicit real and simulation
  times instead.
- `BeOS->Startup(0)` is not a lightweight replacement for the client host; it
  expects the Python scheduler. The probe starts only the Blue/Trinity pieces it
  can own.
- Main-thread continuation and scene update blocking traps are now conditional
  on a live `PyOS` when built with Python support.
- Trinity startup and the standalone bridge need explicit default-visibility C
  exports because `trinity_metal` hides its C++ symbol surface.
- Blue resource paths must use the expected `key=path` form. Supplying plain
  paths is insufficient.
- `BlueAsyncRes::ForceSynchronousLoad()` only marks subsequent loading as
  synchronous; it does not wait for a request already queued by
  `Tr2Effect::SetEffectPathName()`. The probe sets the flag and reloads the
  resource before testing `IsGood()`. Because the Python-enabled helper releases
  the GIL, `Py_Initialize()` must precede that resource operation.
- Procedural texture resources were passing addresses of static objects into a
  resource-manager `unique_ptr`. Allocating those resources on the heap avoids
  deleting static storage during shutdown.
- Metal effect-state teardown must check that its own render context is valid,
  skip stream slot 3 reserved for `METAL_VERTEX_STREAM_DUMMY`, and consume
  returned `ALResult` values.
- The retained Astero hull contains surfaces that must be treated as two-sided
  after open GR2 conversion. Trinity's standard opaque cull state made upper
  hull facets look transparent; the probe effect now sets `CullMode = None` in
  both `Depth` and `Main` so the prepass and color pass agree.
- Validate required model inputs before starting Blue. Rejecting a missing CMF
  after Blue startup but before device creation exited through incomplete static
  teardown with status 139; validating first now returns status 1 cleanly.
- The installed client has a separate
  `tq/EVE.app/Contents/Resources/build/resfileindex_macOS.txt`. It indexes
  compiled Metal effects that are absent from the generic resource index.
- System effects must be synchronously loaded before the standalone probe's
  first frame. The full client normally hides this latency during startup.
- CAS sharpening must be gated on upscaling actually being active. Treating
  "the upscaler has no sharpening" as "run CAS" when no upscaler exists fed a
  missing CAS pass into the postprocess chain and replaced valid scene color
  with black.
- Final fullscreen presentation on Metal must clear stale color/depth
  attachments and restore the fullscreen viewport before the blit.
- Current V5 shaders do not use the legacy bones-first
  `Tr2PerObjectDataSkinned` layout. They consume `EveSpaceObjectVSData` and
  `EveSpaceObjectPSData`; bone transforms live in a global ring buffer addressed
  by `boneOffsets`. Feeding the legacy layout made world-matrix reads consume
  bone rows and produced a radial burst. Publishing the modern structs with
  bone count zero is the correct static-object contract.
- The V5 tangent input is CMF `PackedTangent`, not the conventional
  tangent-plus-handedness vector used by the comparison shader. The bridge keeps
  separate vertex streams so both paths remain valid.

These changes make standalone probing possible, but they should be reviewed
separately before treating them as the final lifecycle contract for an embedded
Trinity host.

## EVE fitting illumination fixture

The authored V5 path now loads `res:/dx9/scene/fitting/fitting.black` instead of
constructing a neutral `EveSpaceScene`. The selected client resources are copied
from SharedCache into the build tree, preserving their logical resource paths:

- `fitting.black`
- `fitting_cube.dds`
- `fitting_cube_blur.dds`
- `fitting_cube_refl.dds`

The current Tranquility fixture decodes to sun direction
`(-0.5392, -0.5392, -0.6470)`, sun color `(2, 2, 2)`, ambient color
`(0.3041, 0.4588, 0.4534)`, and reflection and nebula intensities of `1`. The
scene's normal per-frame constant population supplies these values to `quadv5`;
the probe does not duplicate them in sample-owned shader parameters.

The installed client creates an SH lighting manager at high shader quality, but
the empty fitting scene has no secondary light sources, so its per-object SH
contribution is zero. The useful ambient term for this fixture is the scene's
authored per-frame ambient color plus the fitting reflection cube.

This fixture is accepted only as proof that authored values reach the shader.
It is not a realistic in-space lighting reference: the bridge's per-object SH
coefficients remain zero and its fitting-room background is not representative
input for in-space exposure validation.

## A01 in-space environment fixture

The direct runtime path uses `res:/dx9/scene/universe/a01_cube.black` as its
deterministic authored environment. The build stages its full
first-order graph from SharedCache: the Black, full/reflection/blur/low-detail
cubes, two low-quality nebula maps, overlay, star map, and all three Metal
background containers. `Reports/A01SceneResources.json` records 12 resources,
45,850,599 source bytes, their source indexes, hashed paths, and SHA-256 values.

Black deserialization in the standalone host deliberately suppresses recursive
initialization to avoid yielding into the absent Python scheduler. Consequently
the serialized background effect previously existed but had no prepared shader
or texture providers. The probe now initializes that effect and its texture
parameters explicitly, forces synchronous loads, and rejects missing or invalid
resources before the first frame. This remains probe-local; Trinity does not gain
a generic object-graph walker or scheduler emulation.

The fixture supplies ambient `(0.0941, 0.0706, 0.0431)`, nebula intensity
`1.25`, reflection intensity `1.55`, and
`a01_cube_refl.dds`/`a01_cube_blur.dds` environment inputs. The same selected
scene now drives the Astero's V5 reflection constants and renders the visible
nebula/star background. Its sun direction `(0, -1, 0)` and white color are Black
defaults, and the fixture has no SH manager.

Client code confirms the boundary: `cfg.GetNebula(system,
constellation, region)` selects a universe Black, while scene application adds a
seeded sprite starfield and the live sun object derives distance-dependent color
from star graphics and direction from ballpark position.

## New Eden named-system fixture

`new-eden` specializes the accepted A01 control with installed-client and map
data for solar system `30005286`, constellation `20000773` (EVE), and region
`10000067` (Genesis). The region resolves to `a01_cube.red`, whose packaged Black
is the A01 environment already staged by RC-02. The observer is fixed at New
Eden's Promised Land stargate so star direction and distance are deterministic.

New Eden's star is type `45041`, graphic `21480`, radius `158,400,000`, with
emissive color `(5.0, 4.27451, 2.35294)`. The probe reproduces the client's
`Sun._UpdateSunColor` HSV interpolation at the stargate distance, yielding scene
direction `(-0.780651, 0.147934, 0.607206)` and diffuse color
`(1.500809, 1.391902, 1.103444)`. These values enter the ordinary
`EveSpaceScene` per-frame constants; the Astero shader receives no sample-owned
light override.

The client starfield rule produces `500 + int(250 * securityStatus)` stars. New
Eden's security `0.260554` therefore creates 565 sprites, seeded with its
constellation ID and distributed from 40 to 80 scene units. The original
`spritestars.black`, atlas, palette, and three Metal effect containers are staged
under their logical paths and recorded in `Reports/NewEdenSceneResources.json`.

This accepts the direct scene-lighting contract, including the identity
environment rotation. It does not instantiate the visible sun model, lens flare,
god rays, distant `universe.red` object, object SH, or local lights. Those are
tracked separately rather than approximated as part of the direct sun.

The client-wide default postprocess Black activates Uncharted 2 tone mapping,
histogram-based dynamic exposure, bloom, and film grain. `hdr-exposure` loads
that Black but removes bloom, film grain, and TAA, leaving only the authored tone
and dynamic-exposure contract. Its three compute effects come from the current
client's macOS resource index.

The first exposure attempt stayed black because the standalone host skips the
Python-dependent `TriDevice::OnTick`; consequently Trinity animation time never
advanced and the temporal exposure scalar remained at its zero initializer.
The probe now transfers its explicit simulation clock to `TriDevice` before
`BeginFrame`. Exposure then adapts normally. Very short captures include the
client preset's startup transient, so exposure comparisons use at least 180
warm-up frames. Bloom and film grain remain separate observable rungs.

## Revised rung model

The original ladder treated model submission, HDR, and postprocess as a linear
quality stack. That is useful for machinery bring-up but incorrect for visual
fidelity: background, lighting, material, and render-product contracts jointly
produce the scene consumed by exposure and later effects. The revised rungs
make those prerequisites explicit.

| Rung | Runtime contract | State | Acceptance gate |
| --- | --- | --- | --- |
| 0 | Standalone TrinityAL controls | Accepted | Preserve Fox and box captures. |
| 1 | EVE device shell | Accepted | Clear/present and finite shutdown return zero. |
| 2 | Empty EVE scene driver | Accepted as machinery | Driver validation and execution return zero. |
| 3A | Geometry, topology, UVs, and textures | Accepted | Astero silhouette and material-view captures remain coherent. |
| 3B | Authored material and per-object contract | Partial | Replace remaining bridge defaults with the complete object/material-area contract or prove each default is neutral. |
| 3C | Representative in-space scene and background | Accepted | A01 renders through `EveSpaceScene`; its explicit resource manifest, scene-only capture, and Astero capture pass. |
| 3D | Object lighting contract | Missing | Supply and validate SH coefficients, local/secondary lights, reflection inputs, and direct sun together. |
| 4A | Depth and normal products | Partial | Capture both products directly and validate conventions. |
| 4B | Shadows and AO | Missing | Feed validated products and shadow-caster batches; accept against captures. |
| 4C | Complete HDR scene composition | Missing | Model, background, lighting, and generated products coexist without regression. |
| 5 | Exposure and tone mapping | Machinery accepted, fidelity blocked | Re-run `hdr-exposure` against accepted rung 4C and compare settled captures. |
| 6 | Bloom, film grain, distortion, and volumetrics | Blocked | Add one effect at a time only after rung 5 fidelity acceptance. |
| 7 | Velocity and TAA | Missing | Publish correct current/previous transforms and validate velocity before TAA. |

`hdr-post` and `hdr-exposure` remain useful executable checkpoints. They prove
effect loading, FP16 targets, compute dispatch, tone mapping, and presentation;
they do not advance the direct fidelity path past rung 3C. Missing runtime
assets still return nonzero rather than silently falling back.

Detailed task dependencies and artifact evidence live in
[`eve-runtime-contract-roadmap.md`](eve-runtime-contract-roadmap.md). That
tracker is authoritative for what may be enabled next.

## Verification record

The following checks passed on the host snapshot above:

- focused builds of the converter and all three sample executables;
- the full `all` target;
- 120-frame hello/pyramid smoke run;
- 180-frame standalone Fox and box runs;
- EVE `shell` and `scene` finite-frame runs;
- Draco conversion of the ship to one CMF mesh and a 1024x1024 base-color
  sidecar;
- 180-frame EVE ship run with status 0 and PNG capture;
- SharedCache resolution of the Astero, open GR2 conversion of 7,326 retained
  triangles, and BC1/BC4/BC5/BC7 texture decoding;
- nine deterministic material-view captures, including the 1024x1024 dirt and
  paint masks and 512x512 distortion layer map;
- 180-frame EVE Astero run through a native `Tr2Effect` indexed batch with open
  ring silhouette, intact hull UVs, status 0, and a 1280x960 content capture;
- static Fox rest-pose regression through the same native batch with status 0;
- 180-frame `hdr-blit` checkpoint through FP16 scene color, resolve, and the
  extracted client blit;
- 180-frame `hdr-post` run through FP16 scene color, extracted client tone
  mapping, intermediate output, and final client blit with status 0;
- authored `quadv5` A/B mode with all seven opaque-hull DDS resources reporting
  good, exact SOF material constants, coherent packed-tangent geometry, modern
  V5 per-object constants, and a finite HDR/post capture;
- 180-frame authored V5 run using the exact fitting-scene sun, ambient color,
  reflection intensity, and reflection cube, with capture and clean status 0;
- 180-frame `hdr-exposure` run through the client histogram, merge, and exposure
  compute effects with an advancing standalone animation clock and status 0;
- checksummed staging of the 12-resource, 45,850,599-byte A01 first-order graph;
- scene-only A01, fitting, and empty control captures with status 0;
- 180-frame A01 Astero capture using the authored background and reflection cube;
- 180-frame A01 `hdr-post` and `hdr-exposure` regressions with visible nebula and
  status 0;
- scene-only and 180-frame Astero New Eden captures with exact direct-sun values,
  565 seeded sprite stars, and status 0;
- a settled 180-frame New Eden `hdr-exposure` capture with the named-system
  composition intact;
- capture-owned Cocoa objects drained before Trinity device destruction, fixing
  the long-run window-snapshot shutdown fault;
- missing-model input returns status 1 before Blue startup;
- clean shutdown without the earlier Blue lock-count assertion;
- `git diff --check`;
- no Granny entry in `otool -L` for the EVE sample or Trinity Metal module.

## Next steps

The direct path now takes precedence over additional postprocess checkpoints:

1. Complete the object's lighting contract, beginning with
   `ITr2ShLightingReceiver` and nonzero `EveSpaceObjectPSData` SH coefficients,
   then validate local/secondary lights and direct sun independently.
2. Capture and validate depth and normal products; only then enable shadows and
   AO to reach complete HDR composition.
3. Reaccept dynamic exposure and tone mapping against that composition.
4. Resume bloom, film grain, distortion, volumetrics, velocity, and TAA one
   observable subsystem at a time.
5. Promote finite-frame checkpoints to macOS CI so lifecycle and resource
   regressions are detected automatically.

At each step, preserve the previous rung's image and finite-frame exit status.
The central lesson from this bring-up is that visual quality is downstream of
correct data products and resource ownership. The ladder should continue to
make one new subsystem observable at a time.
