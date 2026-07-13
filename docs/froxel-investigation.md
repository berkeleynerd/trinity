# RC-12B2 Froxel Investigation

## Incident And Scope

Two complete native froxel attempts on the primary M4 Pro stalled AGX and ended
in WindowServer watchdog reboots. Historical evidence is retained at:

```text
/Library/Logs/DiagnosticReports/WindowServer-2026-07-11-213209.ips
/Library/Logs/DiagnosticReports/WindowServer_2026-07-11-213213_mini.userspace_watchdog_timeout.spin
```

The exact original command line was not retained. `R00/R01` therefore reproduce
the known 1280x720 High New Eden Mie/calculate/raymarch/apply boundary without
claiming bit identity.

## Current Evidence

The M4 GPU advertises Tier-2 texture read/write support. Device-free disassembly
of every current high/low/depth permutation found bounds predicates and encoder
boundaries, so no missing M4 feature has been identified.

The audit did find two runtime contract defects:

1. Client calculate/filter/raymarch AIR requires a 2,432-byte object constant
   buffer. Trinity supplied 2,304 bytes, omitted projection and inverse-projection
   matrices, shifted later fields by 128 bytes, and could expose out-of-bounds
   constant reads.
2. Raymarch declares a reflected `8x8x1` threadgroup. Trinity used a stale
   `8x4` dispatch assumption.

Both are corrected independently. Float and uint 3D clears are now bounded, and
Metal binding preflight rejects nil, dummy, wrong-type, wrong-usage, or undersized
required resources. These corrections are candidates, not accepted root causes.

## Lab Design

`BUILD_FROXEL_GPU_LAB` defaults off and all lab targets are excluded from `all`.
`TrinityALFroxelProbe_metal` runs source-controlled synthetic kernels or an
offline-extracted, checksummed client package. The hazardous scene reproducer is
`TrinityALEveSceneProbeFroxelLab_metal`; normal `froxel|all` modes still fail
before Metal startup.

Every run records hardware/boot identity, revision and dirty hash, executable,
dSYM, container and manifest hashes, dimensions, format, dispatch, aliasing,
synchronization, temporal state, encoder labels, pipeline IDs, and complete
binding preflight. State advances through `prepared`, `armed`, `started`,
`submitted`, and `completed`; only worker-side precommit can mark `submitted`.

## Root-Cause Method

Progress from device-free `A00` through synthetic `S10-S15`, isolated client
`C10-C61`, and incident-equivalent `R00/R01`. Vary one factor at a time. A defect
is the watchdog cause only after repeated single-variable failure/success A/Bs
and corroborating AGX/IOGPU, command-buffer, encoder, and binding diagnostics.

## Sacrificial Results

The first `C10 one-threadgroup` attempt on 2026-07-13 is classified as an
invalid harness run. Revision `83ac0addbbcd0d12c91d661b7aeba8e8a6797743`
created a Metal device and loaded the client libraries, but
`BytesPerPixel` did not recognize `MTLPixelFormatR32Float`. CPU initialization
of the synthetic shadow atlas therefore failed before encoder creation,
binding preflight, command-buffer submission, or client-kernel execution. No
AGX/IOGPU fault, watchdog, WindowServer failure, stall, or reboot occurred.

The harness now recognizes scalar `R32Float` and `R32Uint` formats and tests
both with a CPU fill/readback round trip during safe synthetic execution. Run
selection by `latest` uses the ledger preparation timestamp rather than the
lexicographic experiment ID, and collection uses a `log show` compatible time
format while retaining sysdiagnose command output. This result does not change
the 2,432-byte layout or reflected-dispatch hypotheses. Repeat only `C10
one-threadgroup` after deploying the corrected clean revision.

## Promotion Gates

Before returning to the primary host, require all isolated client stages to pass
100 fresh-worker runs, 100-frame chains with Mie and temporal history off/on, and
a 180-frame secondary scene comparison. The signed report must identify commit,
shader hashes, host, OS, options, diagnostics, and results.

Primary promotion is limited to one Low frame, three High frames, then 180 High
frames. Explicit `froxel|all` remains disabled until those gates pass; `auto`
remains off afterward. CP-27 is therefore split into offline/synthetic,
secondary-client, and primary-promotion evidence, with only the first gate
currently available on this host.
