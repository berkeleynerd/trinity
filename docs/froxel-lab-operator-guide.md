# Froxel Lab Operator Guide

## Safety Boundary

Client froxel kernels can stall AGX, freeze WindowServer, and force a reboot. Run
`C10-C61` and `R00-R01` only on recoverable sacrificial hardware with a physical
operator present. Never run them through `launchctl`, remote-only access, or an
automatic restart loop. The production scene probe remains fail-closed.

After the immutable ladder passes, `D00 visual` may be run as a bounded visual
demonstration. It retains the same enrollment, nonce, confirmation, sentinel,
collection, and recovery requirements; it is not part of the promotion ladder.
The lab executable must be configured with `BUILD_DESTINY_INTEGRATION=ON`
against `carbon-destiny` commit `c20c8a6` or a compatible package that exports
the embedded warp and follow contracts.

Use a dedicated local account without iCloud, credentials, private data, network
mounts, or attached writable media. Verify a disposable installation or backup,
FileVault recovery credentials, and at least 75 GiB free. A matching M4 Pro,
macOS `26.5.2 (25F84)`, Metal 4, and Xcode `26.6` is preferred.

## Prepare The Host

Check out the exact promoted revision and make the EVE SharedCache available only
as an external input. Then run:

```sh
cd /path/to/trinity
tools/froxel_lab/bootstrap_sacrificial.sh
```

This configures `BUILD_FROXEL_GPU_LAB=ON`, builds the excluded lab executables,
validates manifests, and performs device-free AIR audits. It does not execute a
client kernel. Compare `VolumetricResources.json` and the bootstrap inventory
with the primary host before continuing.

## Experiment Order

Run the immutable catalog in order: `A00`, all profiles of `S10-S15`, all
profiles of `C10-C61`, then `R00` and `R01`. Only one run may be open. Collect
and close every profile before advancing.

Safe experiments use the same wrapper without client authorization:

```sh
tools/froxel_lab/run_experiment.sh A00 all-permutations
tools/froxel_lab/run_experiment.sh S10 odd
```

Immediately before a hazardous run, close other applications and run locally:

```sh
sudo -v
tools/froxel_lab/run_experiment.sh C20 one-threadgroup
```

The wrapper requires a hardware-bound 24-hour enrollment, one-use nonce,
`TRINITY_FROXEL_GPU_LAB=I_ACKNOWLEDGE_GPU_RESET_RISK`, controlling TTY, and an
exact host/run confirmation phrase. A durable ledger is fsynced under
`~/Library/Logs/TrinityFroxelLab/runs/<run-id>/` before submission.

## Lockup Or Reboot

Wait for watchdog recovery if possible. If the machine remains unusable, power
cycle it manually. After local login, do not rerun the experiment. Run:

```sh
cd /path/to/trinity
tools/froxel_lab/recover_and_collect.sh latest
```

The collector detects a changed boot UUID and submitted-without-completion
ledger, gathers matching `.ips`, `.spin`, `.panic`, `.diag`, unified logs,
hardware state, symbols, and a full `sysdiagnose`, then closes the run. The
sentinel never restarts the worker.

## Export Evidence

Attach removable/shared storage only after hazardous execution and collection:

```sh
tools/froxel_lab/export_evidence.sh latest /Volumes/LAB-EVIDENCE
```

The focused archive redacts usernames, home paths, serials, UUIDs, and network
identifiers. The full sysdiagnose and symbols archives are unredacted and
sensitive. Verify `<run-id>-SHA256SUMS`; nothing is uploaded automatically.

On the analysis host:

```sh
python3 tools/froxel_lab/froxel_lab.py analyze \
  --bundle /path/to/<run-id>-focused.tar.gz
```

Do not test a subsequent profile until the prior bundle is reviewed and the run
is closed.
