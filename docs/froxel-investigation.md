# RC-12B2 Froxel Investigation

## Incident scope

Two complete native froxel attempts on the original M4 Pro host stalled AGX
and ended in WindowServer watchdog reboots on 2026-07-11. The original command
line was not retained, so the later `R00` and `R01` scenes are
incident-equivalent boundaries rather than bit-identical reproductions.

The investigation therefore separated two questions: whether Trinity violated
the current client shader contract, and whether any corrected contract defect
was the cause of the original watchdog. The first question is answered. The
second is not: no single-variable repeated A/B tied either correction to the
M4 incident with corroborating AGX/IOGPU diagnostics.

## Corrected contracts

Device-free AIR inspection identified two concrete renderer defects:

1. The calculate, filter, and raymarch containers require a 2,432-byte object
   constant buffer. Trinity supplied 2,304 bytes, omitted `ProjectionMatrix`
   and `InverseProjectionMatrix`, and shifted all later fields by 128 bytes.
   `FroxelShaderLayouts.h` now expresses the reflected 2,432-byte layout and
   compile-time checks every material offset. Per-dispatch data is zeroed and
   both projection matrices are populated explicitly.
2. Raymarch declares an `8x8x1` threadgroup. Trinity used a stale hard-coded
   `8x4` dispatch assumption. The renderer now obtains the threadgroup from the
   effect reflection and computes all three dispatch dimensions from it.

The promotion run also exposed a third backend mismatch. Froxel allocation
clears a 3D texture, but `MetalWorkQueue::ClearTexture` asserted that every
target was `MTLTextureType2D`. Dedicated bounded float and uint 3D clear
kernels now dispatch over width, height, and depth. Unsupported texture types
still fail rather than falling through.

Normal startup remains fail-closed. `froxel` and `all` require the explicit
`--enable-froxels` host option; the bridge independently requires its host to
call `TrinityStandaloneProbeSetFroxelRenderingEnabled` before configuration.
`auto` remains `off` because the New Eden fixture has no authored global fog
object and the visual fog values remain sample-owned capability inputs.

## Ladder result

The sacrificial M2 ladder completed in immutable order. The latest A00 run,
all five profiles for every S10-S15 stage, all five profiles for every
C10/C20/C30/C40/C50/C51/C60/C61 stage, and the final R00 and R01 runs reached
`completed`, then `collected`, then `closed`. Earlier harness and scene
integration failures remain in the external ledgers; they were not rewritten
as successful runs. No SharedCache payload or evidence bundle is committed.

The accepted final incident-equivalent runs are:

- `R00-incident-20260713T202404Z`, temporal history off.
- `R01-incident-20260713T203534Z`, temporal history on.

Both use reflected dispatch and a `160x90x128` R11G11B10 froxel volume. R01
completed with worker status 0 and one-frame HDR output; TAA remained off so
the R00/R01 difference stayed limited to froxel temporal history.

## Standalone visual promotion

The experiment harness was removed from the visual demo path. A first bounded
640x400, 240-frame, all-effects run completed the froxel stages but logged the
2D-only clear assertion described above. After the bounded 3D clear fix, the
identical run exited 0 with no assertion, Metal error, timeout, or watchdog:

```text
volume=160x100x128
threadgroup=8x8x1
dispatch=20x13x1
stages=calculate filter raymarch apply
```

The final full-screen 2940x1912 demo then initialized the same native
calculate/filter/raymarch/apply/Mie effects with temporal froxels, High TAA,
HDR exposure and tone mapping, bloom, film grain, distortion, authored
lighting and shadows, Silk, celestials, and the EVE Gate journey. It produced
no assertion or Metal error during operator inspection.

This accepts global froxel execution as a working M2 Metal capability. It does
not establish New Eden fog fidelity and does not identify the cause of the
original M4 watchdog without the missing repeated single-variable A/B.
