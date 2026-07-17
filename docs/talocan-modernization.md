# Talocan tde1 Modernization Lane

Stage-1 rehearsal for the asset-modernization pipeline: the Talocan tde1
wreck (a single-area pre-V5 dormant hull) renders through the probe's real
SOF/eve-v5 material path, and its textures can be swapped for a modernized
set at run time with a byte-deterministic A/B rig around the swap.

## What landed

- `--asset talocan` in the EVE scene probe (legacy scene-construction
  lane). The SOF hull (`tde1_t1_wreck.black`) and faction
  (`talocanbase.black`) drive areas, textures, and glow; the probe treats
  the model's only GR2 group as the hull and tolerates texture slots the
  hull's area leaves unset.
- The full ship-class family on the same machinery: `talocan-frigate`
  (tf1), `talocan-cruiser` (tc1), and `talocan-battleship` (tb1). All
  four wrecks are single group-0 meshes with the identical seven-map
  closure; every hull reached first light with zero FAILED slots. The
  modernization workspace and `--sof-texture-set` rewrite remain
  rehearsed on tde1 (the rewrite itself is path-generic, so a family
  workspace is a staging-lane clone away).
- Staging: `TrinityEveSceneProbeTalocanWreckAssets` pulls the GR2, seven
  authored maps, and the SOF hull/faction/race blacks from the SharedCache
  into the ignored build tree (ledger: "Talocan tde1 wreck").
- `tools/rgba_to_dds` (`TrinityRgbaToDds`): PNG or raw-RGBA in, legacy
  DDS out (BC1/BC3 via stb_dxt, `--uncompressed` escape hatch, full mip
  chain, `--no-mips`). `TrinityDdsToRgba` gained the DX10 BC1-BC5 formats
  so every tde1 original decodes.
- `--sof-texture-set original|modernized`: opaque hull-area texture paths
  rewrite from `res:/<rest>` to `res:/modernized/<rest>`. Parallel prefix,
  not overlay — a missing override fails loudly at texture validation.
  Capture names gain `_tex-modernized`.

## Authoritative texture-slot map (from first light)

The tde1 hull area binds seven slots; `DustNoiseMap` is the global
fallback and is never rewritten:

| Slot | Original resource |
| --- | --- |
| AlbedoMap | `res:/dx9/model/ship/talocan/destroyer/tde1/tde1_t1_wreck_a.dds` |
| NormalMap | `..._n.dds` |
| RoughnessMap | `..._r.dds` |
| MaterialMap | `..._m.dds` |
| GlowMap | `..._g.dds` (shares a tiny placeholder payload with `_p3`) |
| DirtMap | `..._d.dds` |
| PaintMaskMap | `..._p3.dds` |
| DustNoiseMap | `res:/texture/global/black.dds` (global, not per-hull) |

No full-resolution `_o` map exists for this hull.

## Workspace rules (hard constraints)

Everything derived from EVE client bytes — staged originals, decoded RGBA,
modernized DDS, tinted PNGs — lives in the ignored build tree or in the
out-of-repo workspace `~/TalocanModernization/tde1_t1_wreck/`. Nothing
EVE-derived enters git. The bootstrap refuses a workspace that resolves
inside any git checkout. Real replacement art (authored from scratch) is
not a client derivative, but it still stages through the same out-of-repo
workspace so the repo carries only code.

## Workflow

```sh
BUILD=.cmake-build-arm64-osx-debug
PROBE=$BUILD/samples/eve_scene_probe/Debug/TrinityALEveSceneProbe_metal_debug

# 1. Seed the workspace (tint-spike validation set; replace PNGs with real
#    art later, same names):
python3 samples/eve_scene_probe/bootstrap_talocan_modern_workspace.py \
    --dds-to-rgba $BUILD/tools/dds_to_rgba/Debug/TrinityDdsToRgba_debug \
    --originals-dir $BUILD/samples/eve_scene_probe/Assets/dx9/model/ship/talocan/destroyer/tde1

# 2. Reconfigure (the workspace is scanned at configure time), then:
cmake --build $BUILD --config Debug \
    --target TrinityEveSceneProbeTalocanModernAssets TrinityEveSceneProbeRuntimeAssets

# 3. A/B at hdr-finish (run from the Debug directory; identical flags,
#    same prefix — the _tex- token separates the pair):
FLAGS=(--windowed 1024x768 --frames 180 --quality-rung hdr-finish
       --asset talocan --material-mode eve-v5 --scene-construction legacy
       --taa off --local-lights off --attachments off --decals off
       --engines off --distortion off --motion static --deterministic-evidence
       --background-capture)
$PROBE $FLAGS --capture-prefix captures/tde1-ab
$PROBE $FLAGS --sof-texture-set modernized --capture-prefix captures/tde1-ab

# 4. Verdicts (stdlib-only; sips does the PNG->BMP conversion):
python3 samples/eve_scene_probe/compare_talocan_ab.py --mode aa --a <runA1> --b <runA2>
python3 samples/eve_scene_probe/compare_talocan_ab.py --mode ab --a <original> --b <modernized>
```

`aa` demands a (near) byte-identical pair — the determinism floor under
the rig. `ab` demands a difference that is more than codec noise, less
than a scene-wide change, and localized (the hull's bounding box).

`--background-capture` orders the probe window front without activating
the app, so interactive runs do not steal keyboard focus. Verified
byte-identical to a focused run on the same binary (aa: 0.0 / 0.0);
determinism only holds within one binary — captures from different
builds can carry sparse sub-0.2% float drift.

## Accepted evidence (2026-07-17)

- First light: model and hdr-finish rungs exit 0, all seven slots bound,
  zero FAILED slots, hull lit and shadowed (4 cascades x 1 batch).
- A/A at hdr-finish (`--taa off`): `diff_pixel_fraction = 0.0`,
  `mean_abs_delta = 0.0` — byte-identical.
- A/B tint spike: `diff_pixel_fraction = 0.0204`, bbox tight around the
  hull; scene pixels byte-match.
- Astero regression after every probe change: exit 0, unchanged area
  configuration, `accepted-cascades=3 batches=6`.

## Formation composition (legacy lane)

`--formation-ships id[,id...]` renders additional SOF hulls beside the
primary model: each wingman carries its own geometry, hull material, and
per-object transforms, riding the primary root transform at a computed
line-abreast offset — so a formation follows the primary through journey
legs, warp included. Wingmen contribute directional-shadow batches (the
contract expectation scales with the hulls present) and are excluded
from raytracing, attachments, decals, and distortion. Legacy-lane, SOF
assets, eve-v5 only.

`--chase-distance M` / `--chase-height M` re-frame the tour chase rig,
whose authored constants (150/55) shoulder-frame an Astero-sized hull;
a Talocan battleship swallows the default rig whole. Zero keeps the
authored tour behavior exactly.

The accepted four-wreck EVE Gate shot: warp the family to the gate on
the tour route and capture the post-arrival gate-facing window (the
warp leg completes near frame 1620 of the journey; the tunnel resets
there and the fleet parks at the warp-in point):

```sh
$PROBE --windowed 1600x1000 --frames 1700 --quality-rung hdr-finish \
    --asset talocan-battleship \
    --formation-ships talocan-cruiser,talocan,talocan-frigate \
    --scene-construction legacy --scene-fixture new-eden \
    --lighting-view combined --sh-source new-eden-celestials \
    --local-lights off --local-shadows off --attachments off --decals off \
    --engines off --distortion off --reflection-source dynamic \
    --reflection-correction client --shadows high --ao high --ao-method cortao \
    --composition system --planet-layers all --sun-effects all \
    --dynamic-exposure client --bloom client --film-grain client --taa high \
    --motion static --ballpark warp --warp-target evegate \
    --ballpark-frame chase --chase-distance 5200 --chase-height 1400 \
    --celestial-ballpark natural --eve-gate authored --warp-tunnel authored \
    --celestial-anchor stargate --volumetrics all --volumetric-quality high \
    --enable-froxels --background-capture \
    --capture-prefix captures/formation/fleet --capture-frames 1626,1650,1680
```

A static family portrait (no ballpark) also composes: the same
formation flags with `--scene-fixture new-eden --composition system
--eve-gate authored --celestial-anchor stargate` and no ballpark
render all four hulls lit three-quarter on the default model camera.

For a posed static shot, `--camera-azimuth DEG --camera-elevation DEG
--camera-distance M` re-aim the model camera around the formation
(azimuth rotates from the authored -Z eye; zero distance keeps the
authored pose). The accepted no-warp gate composition places the fleet
at the stargate anchor and swings the camera until the authored gate
sits behind the line: `--camera-azimuth -119 --camera-elevation 2
--camera-distance 4300` (the anchor's gate bearing is close to the sun
direction, so dead-on aim silhouettes the hulls; ~10-20 degrees off
keeps them lit).

## Known limitations

- `TrinityRgbaToDds` mip generation is a byte-space 2x2 box (not
  gamma-correct); fine for validation, revisit for beauty-grade art.
- Auto format picks BC1 when a PNG carries no translucency; authored art
  that needs alpha (or BC5-style normals) should ship alpha content or a
  `--format` override.
- Raytraced geometry with a boosterless hull is untested (guard relaxed,
  materials array still lists the booster slot).
- Authored decals, attachments, and local-light rigs remain
  Astero-hardcoded and stay off/astero-gated for talocan.
