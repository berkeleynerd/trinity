#!/usr/bin/env python3
"""Crash-surviving orchestration for the macOS froxel GPU safety lab.

The default commands either inspect files or run source-controlled synthetic
kernels. Client kernels are additionally gated by sacrificial-host enrollment,
a one-use nonce, an interactive controlling TTY, and an explicit environment
acknowledgement.
"""

from __future__ import annotations

import argparse
import datetime as dt
import getpass
import hashlib
import json
import os
import platform
import re
import secrets
import shutil
import signal
import socket
import subprocess
import sys
import tarfile
import tempfile
import time
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
CATALOG_PATH = Path(__file__).with_name("experiments.json")
LAB_ROOT = Path(os.environ.get(
    "TRINITY_FROXEL_LAB_ROOT",
    "~/Library/Logs/TrinityFroxelLab",
)).expanduser()
RUNS_ROOT = LAB_ROOT / "runs"
ENROLLMENT_PATH = LAB_ROOT / "enrollment.json"
RISK_ACK = "I_ACKNOWLEDGE_GPU_RESET_RISK"
REPORT_EXTENSIONS = {".ips", ".spin", ".panic", ".diag", ".crash", ".hang"}

CLIENT_STAGE_SPECS = {
    "apply_vs": {
        "container": "applyfroxels.sm_hi", "permutation": 0, "stage": 0,
        "function": "mainVS", "threadgroup": (1, 1, 1), "bufferSize": None,
    },
    "apply_ps": {
        "container": "applyfroxels.sm_hi", "permutation": 0, "stage": 1,
        "function": "mainPS", "threadgroup": (1, 1, 1), "bufferSize": 1888,
    },
    "mie": {
        "container": "updatemieenvironmentmap.sm_hi", "permutation": 0, "stage": 2,
        "function": "mainCS", "threadgroup": (8, 8, 1), "bufferSize": 24,
    },
    "calculate": {
        "container": "calculatefroxels.sm_hi", "permutation": 0, "stage": 2,
        "function": "mainCS", "threadgroup": (4, 4, 4), "bufferSize": 2432,
    },
    "filter": {
        "container": "filterfroxels.sm_hi", "permutation": 0, "stage": 2,
        "function": "mainCS", "threadgroup": (4, 4, 4), "bufferSize": 2432,
    },
    "raymarch_out": {
        "container": "raymarchfroxels.sm_hi", "permutation": 0, "stage": 2,
        "function": "mainCS", "threadgroup": (8, 8, 1), "bufferSize": 2432,
    },
    "raymarch_in": {
        "container": "raymarchfroxels.sm_hi", "permutation": 1, "stage": 2,
        "function": "mainCS", "threadgroup": (8, 8, 1), "bufferSize": 2432,
    },
}


class LabError(RuntimeError):
    pass


def utc_now() -> dt.datetime:
    return dt.datetime.now(dt.timezone.utc)


def iso_time(value: dt.datetime | None = None) -> str:
    return (value or utc_now()).isoformat(timespec="microseconds").replace("+00:00", "Z")


def parse_time(value: str) -> dt.datetime:
    return dt.datetime.fromisoformat(value.replace("Z", "+00:00"))


def run_command(command, *, check=False, timeout=None, text=True, env=None):
    try:
        return subprocess.run(
            command,
            check=check,
            capture_output=True,
            text=text,
            timeout=timeout,
            env=env,
        )
    except (OSError, subprocess.SubprocessError) as exc:
        if check:
            raise LabError("command failed: {}: {}".format(" ".join(map(str, command)), exc)) from exc
        return None


def command_output(command, default="") -> str:
    result = run_command(command)
    if not result or result.returncode != 0:
        return default
    return result.stdout.strip()


def fsync_directory(path: Path) -> None:
    descriptor = os.open(str(path), os.O_RDONLY)
    try:
        os.fsync(descriptor)
    finally:
        os.close(descriptor)


def atomic_write(path: Path, data: bytes, mode=0o600) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(".{}.{}.tmp".format(path.name, os.getpid()))
    with open(temporary, "wb") as output:
        os.fchmod(output.fileno(), mode)
        output.write(data)
        output.flush()
        os.fsync(output.fileno())
    os.replace(temporary, path)
    fsync_directory(path.parent)


def atomic_write_json(path: Path, value, mode=0o600) -> None:
    payload = json.dumps(value, indent=2, sort_keys=True).encode("utf-8") + b"\n"
    atomic_write(path, payload, mode=mode)


def load_json(path: Path):
    with open(path, "r", encoding="utf-8") as source:
        return json.load(source)


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with open(path, "rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def file_record(path: Path) -> dict:
    resolved = path.resolve()
    return {
        "path": str(resolved),
        "size": resolved.stat().st_size,
        "sha256": sha256_file(resolved),
    }


def load_catalog() -> dict:
    catalog = load_json(CATALOG_PATH)
    identifiers = [item["id"] for item in catalog["experiments"]]
    if len(identifiers) != len(set(identifiers)):
        raise LabError("experiments.json contains duplicate experiment IDs")
    return catalog


def find_experiment(identifier: str) -> tuple[dict, int, dict]:
    catalog = load_catalog()
    for index, experiment in enumerate(catalog["experiments"]):
        if experiment["id"] == identifier:
            return experiment, index, catalog
    raise LabError("unknown experiment '{}'".format(identifier))


def boot_session_uuid() -> str:
    value = command_output(["/usr/sbin/sysctl", "-n", "kern.bootsessionuuid"])
    return value or "unavailable"


def os_build() -> str:
    return command_output(["/usr/bin/sw_vers", "-buildVersion"], platform.version())


def hardware_facts() -> dict:
    result = run_command(["/usr/sbin/system_profiler", "SPHardwareDataType", "-json"])
    data = {}
    if result and result.returncode == 0:
        try:
            entries = json.loads(result.stdout).get("SPHardwareDataType", [])
            data = entries[0] if entries else {}
        except json.JSONDecodeError:
            data = {}
    registry = command_output([
        "/usr/sbin/ioreg", "-rd1", "-c", "IOPlatformExpertDevice",
    ])
    registry_uuid = ""
    match = re.search(r'"IOPlatformUUID"\s*=\s*"([^"]+)"', registry)
    if match:
        registry_uuid = match.group(1)
    return {
        "model": data.get("machine_model", platform.machine()),
        "chip": data.get("chip_type", data.get("cpu_type", "unknown")),
        "memory": data.get("physical_memory", "unknown"),
        "platformUUID": registry_uuid,
        "osBuild": os_build(),
    }


def hardware_fingerprint() -> str:
    facts = hardware_facts()
    canonical = json.dumps(facts, sort_keys=True, separators=(",", ":")).encode("utf-8")
    return sha256_bytes(canonical)


def git_metadata() -> dict:
    revision = command_output(["/usr/bin/git", "rev-parse", "HEAD"], "unavailable")
    branch = command_output(["/usr/bin/git", "branch", "--show-current"], "detached")
    status = run_command(["/usr/bin/git", "status", "--porcelain=v1", "-z"], text=False)
    diff = run_command(["/usr/bin/git", "diff", "--binary", "HEAD"], text=False)
    payload = b""
    if status and status.returncode == 0:
        payload += status.stdout
    if diff and diff.returncode == 0:
        payload += diff.stdout
    return {
        "revision": revision,
        "branch": branch,
        "dirty": bool(payload),
        "dirtyStateSha256": sha256_bytes(payload),
    }


def disk_free_bytes(path: Path) -> int:
    return shutil.disk_usage(path).free


def current_inventory() -> dict:
    facts = hardware_facts()
    displays = run_command(["/usr/sbin/system_profiler", "SPDisplaysDataType", "-json"])
    xcode = command_output(["/usr/bin/xcodebuild", "-version"], "unavailable")
    metal = command_output(["/usr/bin/xcrun", "metal", "--version"], "unavailable")
    enrollment = load_json(ENROLLMENT_PATH) if ENROLLMENT_PATH.exists() else None
    return {
        "schemaVersion": 1,
        "capturedAt": iso_time(),
        "hostname": socket.gethostname(),
        "user": getpass.getuser(),
        "hardware": {
            "model": facts["model"],
            "chip": facts["chip"],
            "memory": facts["memory"],
            "fingerprintSha256": hardware_fingerprint(),
        },
        "os": {
            "version": platform.mac_ver()[0],
            "build": facts["osBuild"],
            "bootSessionUUID": boot_session_uuid(),
        },
        "toolchain": {"xcode": xcode, "metal": metal},
        "metalInventory": json.loads(displays.stdout) if displays and displays.returncode == 0 else {},
        "repository": git_metadata(),
        "freeBytes": disk_free_bytes(REPO_ROOT),
        "enrollment": enrollment,
    }


class _ControllingTTY:
    def __init__(self, reader, writer):
        self.reader = reader
        self.writer = writer

    def isatty(self) -> bool:
        return self.reader.isatty() and self.writer.isatty()

    def write(self, text: str) -> int:
        written = self.writer.write(text)
        self.writer.flush()
        return written

    def readline(self) -> str:
        return self.reader.readline()

    def close(self) -> None:
        try:
            self.reader.close()
        finally:
            self.writer.close()


def require_controlling_tty() -> object:
    try:
        reader = open("/dev/tty", "r", encoding="utf-8", buffering=1)
        try:
            writer = open("/dev/tty", "w", encoding="utf-8", buffering=1)
        except OSError:
            reader.close()
            raise
    except OSError as exc:
        raise LabError("a controlling TTY is required") from exc
    tty = _ControllingTTY(reader, writer)
    if not tty.isatty():
        tty.close()
        raise LabError("/dev/tty is not an interactive terminal")
    return tty


def enrollment_record(require_valid=True) -> dict:
    if not ENROLLMENT_PATH.exists():
        raise LabError("this host is not enrolled; run 'froxel_lab.py enroll --role sacrificial'")
    record = load_json(ENROLLMENT_PATH)
    if record.get("role") != "sacrificial":
        raise LabError("host enrollment is not sacrificial")
    if record.get("hardwareFingerprintSha256") != hardware_fingerprint():
        raise LabError("host hardware fingerprint differs from enrollment")
    if require_valid and parse_time(record["expiresAt"]) <= utc_now():
        raise LabError("sacrificial-host enrollment expired")
    return record


def cmd_inventory(args) -> int:
    inventory = current_inventory()
    if args.output:
        atomic_write_json(Path(args.output), inventory, mode=0o644)
    print(json.dumps(inventory, indent=2, sort_keys=True))
    return 0


def cmd_enroll(args) -> int:
    if args.role != "sacrificial":
        raise LabError("only the explicit sacrificial role is accepted")
    if disk_free_bytes(REPO_ROOT) < 75 * 1024 ** 3:
        raise LabError("sacrificial enrollment requires at least 75 GiB free")
    tty = require_controlling_tty()
    try:
        short_fingerprint = hardware_fingerprint()[:12]
        phrase = "ENROLL {} {} AS SACRIFICIAL".format(socket.gethostname(), short_fingerprint)
        tty.write("Type this phrase exactly to enroll this host for 24 hours:\n{}\n> ".format(phrase))
        entered = tty.readline().rstrip("\n")
        if entered != phrase:
            raise LabError("enrollment confirmation did not match")
    finally:
        tty.close()
    now = utc_now()
    record = {
        "schemaVersion": 1,
        "role": "sacrificial",
        "hostname": socket.gethostname(),
        "hardwareFingerprintSha256": hardware_fingerprint(),
        "enrolledAt": iso_time(now),
        "expiresAt": iso_time(now + dt.timedelta(hours=24)),
        "catalogSha256": sha256_file(CATALOG_PATH),
    }
    atomic_write_json(ENROLLMENT_PATH, record)
    print("Enrolled sacrificial host through {}".format(record["expiresAt"]))
    return 0


def run_directories() -> list[Path]:
    if not RUNS_ROOT.exists():
        return []

    def prepared_key(path: Path) -> tuple[float, str]:
        path_ledger = ledger_path(path)
        if path_ledger.is_file():
            try:
                prepared = load_json(path_ledger).get("preparedAt")
                if prepared:
                    return parse_time(prepared).timestamp(), path.name
            except (LabError, OSError, ValueError, TypeError):
                pass
        return path.stat().st_mtime, path.name

    return sorted((path for path in RUNS_ROOT.iterdir() if path.is_dir()), key=prepared_key)


def resolve_run(value: str) -> Path:
    directories = run_directories()
    if value == "latest":
        if not directories:
            raise LabError("no froxel-lab runs exist")
        return directories[-1]
    path = RUNS_ROOT / value
    if not path.is_dir():
        raise LabError("run '{}' does not exist".format(value))
    return path


def ledger_path(run_dir: Path) -> Path:
    return run_dir / "ledger.json"


def transition(run_dir: Path, state: str, **details) -> dict:
    path = ledger_path(run_dir)
    ledger = load_json(path)
    event = {
        "state": state,
        "wallTime": iso_time(),
        "monotonicSeconds": time.monotonic(),
        "bootSessionUUID": boot_session_uuid(),
    }
    event.update(details)
    ledger.setdefault("stateTransitions", []).append(event)
    ledger["currentState"] = state
    ledger[state + "At"] = event["wallTime"]
    atomic_write_json(path, ledger)
    return ledger


def latest_unclosed_run() -> Path | None:
    for run_dir in reversed(run_directories()):
        path = ledger_path(run_dir)
        if path.exists() and load_json(path).get("currentState") != "closed":
            return run_dir
    return None


def closed_profiles(experiment_id: str) -> set[str]:
    profiles = set()
    for run_dir in run_directories():
        path = ledger_path(run_dir)
        if not path.exists():
            continue
        ledger = load_json(path)
        if ledger.get("experimentId") == experiment_id and ledger.get("currentState") == "closed":
            profiles.add(ledger.get("profile"))
    return profiles


def validate_progression(experiment: dict, experiment_index: int, catalog: dict, profile: str) -> None:
    active = latest_unclosed_run()
    if active:
        raise LabError("run '{}' must be collected and closed before preparing another".format(active.name))
    if profile not in experiment["profiles"]:
        raise LabError("profile '{}' is not defined for {}".format(profile, experiment["id"]))
    if experiment_index == 0:
        return
    previous = catalog["experiments"][experiment_index - 1]
    missing = set(previous["profiles"]) - closed_profiles(previous["id"])
    if missing:
        raise LabError(
            "{} profiles must close before {}: {}".format(
                previous["id"], experiment["id"], ", ".join(sorted(missing))))


def default_binary(kernel_set: str) -> Path:
    build = Path(os.environ.get(
        "TRINITY_FROXEL_BUILD_DIR",
        REPO_ROOT / ".cmake-build-arm64-osx-debug",
    ))
    if kernel_set == "client-scene":
        return build / "samples/eve_scene_probe/Debug/TrinityALEveSceneProbeFroxelLab_metal"
    return build / "tools/froxel_lab/Debug/TrinityALFroxelProbe_metal"


def resolve_executable(path: Path) -> Path:
    candidates = [path]
    if path.suffix:
        candidates.append(path.with_name(path.stem + "_debug" + path.suffix))
    else:
        candidates.append(path.with_name(path.name + "_debug"))
    for candidate in candidates:
        if candidate.is_file() and os.access(candidate, os.X_OK):
            return candidate.resolve()
    raise LabError("lab binary is missing or not executable; checked: {}".format(
        ", ".join(str(candidate) for candidate in candidates)))


def default_effect_root() -> Path:
    build = Path(os.environ.get(
        "TRINITY_FROXEL_BUILD_DIR",
        REPO_ROOT / ".cmake-build-arm64-osx-debug",
    ))
    return build / (
        "samples/eve_scene_probe/Assets/graphics/effect.metal/managed/space/"
        "specialfx/volumetric/fog"
    )


def manifest_effect_records(effect_root: Path) -> list[dict]:
    if not effect_root.is_dir():
        raise LabError("froxel effect root is missing: {}".format(effect_root))
    files = sorted(path for path in effect_root.glob("*.sm_*") if path.is_file())
    if not files:
        raise LabError("no Metal froxel effect containers found under {}".format(effect_root))
    return [file_record(path) for path in files]


def extract_client_package(effect_root: Path, output_dir: Path) -> Path:
    """Extract the audited high-tier functions without creating an MTLDevice."""
    sys.path.insert(0, str(REPO_ROOT / "shadercompiler/python"))
    from shadercompiler.effectinfo import EffectInfo  # pylint: disable=import-outside-toplevel

    output_dir.mkdir(parents=True, mode=0o700, exist_ok=True)
    package = {
        "schemaVersion": 1,
        "createdAt": iso_time(),
        "deviceCreated": False,
        "effectRoot": str(effect_root),
        "stages": {},
    }
    containers = {}
    with tempfile.TemporaryDirectory(prefix="froxel-client-package-") as temporary_name:
        temporary = Path(temporary_name)
        for stage_name, spec in CLIENT_STAGE_SPECS.items():
            container = (effect_root / spec["container"]).resolve()
            if not container.is_file():
                raise LabError("client package source is missing: {}".format(container))
            containers.setdefault(spec["container"], file_record(container))
            effect = EffectInfo(str(container))
            if effect._version != 15:
                raise LabError("{} is effect version {}, expected 15".format(container, effect._version))
            shader = effect.get_shader(spec["permutation"])
            technique = next((item for item in shader.techniques if item.name == "Main"), None)
            if not technique or len(technique.passes) != 1:
                raise LabError("{} does not contain one Main pass".format(container))
            stage = technique.passes[0].stages.get(spec["stage"])
            if not stage or not stage.shader_data:
                raise LabError("{} is missing retained stage {} bytecode".format(
                    container, spec["stage"]))
            if tuple(stage.thread_group_size) != spec["threadgroup"]:
                raise LabError("{} threadgroup {} differs from expected {}".format(
                    stage_name, stage.thread_group_size, spec["threadgroup"]))

            disassembly = disassemble_metallib(stage.shader_data, temporary)
            air = inspect_air(disassembly)
            expected_buffer = spec["bufferSize"]
            if expected_buffer and expected_buffer not in air["bufferSizes"]:
                raise LabError("{} lacks the expected {}-byte constant buffer".format(
                    stage_name, expected_buffer))
            if spec["stage"] == 2 and not any(air["boundsPredicates"].values()):
                raise LabError("{} contains no observable AIR bounds predicate".format(stage_name))

            blob_name = "{}-{}.metallib".format(stage_name, sha256_bytes(stage.shader_data)[:16])
            blob_path = output_dir / blob_name
            atomic_write(blob_path, stage.shader_data)
            package["stages"][stage_name] = {
                "blob": blob_name,
                "bytes": len(stage.shader_data),
                "sha256": sha256_bytes(stage.shader_data),
                "function": spec["function"],
                "stage": spec["stage"],
                "permutation": spec["permutation"],
                "threadgroup": list(stage.thread_group_size),
                "registers": [
                    {
                        "type": item.register_type,
                        "index": item.register_index,
                        "space": item.register_space,
                        "arrayCount": item.array_count,
                    }
                    for item in stage.registers
                ],
                "resources": [vars(item) for item in stage.resources],
                "uavs": [vars(item) for item in stage.uavs],
                "air": air,
                "sourceContainer": spec["container"],
                "sourceContainerSha256": containers[spec["container"]]["sha256"],
            }
    package["containers"] = [containers[name] for name in sorted(containers)]
    package_path = output_dir / "client-package.json"
    atomic_write_json(package_path, package)
    return package_path


def build_worker_command(experiment: dict, profile: str, args, run_dir: Path) -> list[str]:
    if experiment["id"] == "A00":
        return [
            sys.executable,
            str(Path(__file__).resolve()),
            "audit",
            "--effect-root", str(args.effect_root),
            "--report", str(run_dir / "air-audit.json"),
        ]
    binary = resolve_executable(Path(args.binary or default_binary(experiment["kernelSet"])))
    if experiment["kernelSet"] == "client-scene":
        command = [
            str(binary),
            "--windowed", "1280x720",
            "--background-capture",
            "--frames", "1",
            "--quality-rung", "hdr-finish",
            "--asset", "astero",
            "--scene-fixture", "new-eden",
            "--composition", "cinematic",
            "--volumetrics", "froxel",
            "--volumetric-quality", "high",
            "--client-kernels",
            "--froxel-lab-ledger", str(ledger_path(run_dir)),
            "--capture-prefix", str(run_dir / "capture/incident"),
        ]
        if experiment.get("temporal") == "on":
            command.extend(["--taa", "high"])
        else:
            command.extend(["--taa", "off"])
        return command

    dimensions = load_catalog()["profiles"][profile]["dimensions"]
    command = [
        str(binary),
        "--kernel-set", experiment["kernelSet"],
        "--stage", experiment["stage"],
        "--dimensions", dimensions,
        "--format", args.format,
        "--temporal", args.temporal or experiment.get("temporal", "off"),
        "--alias", args.alias or experiment.get("alias", "out-of-place"),
        "--sync", args.sync,
        "--dispatch", args.dispatch,
        "--iterations", str(args.iterations),
        "--report", str(run_dir / "probe-report.json"),
        "--ledger", str(ledger_path(run_dir)),
    ]
    if experiment["kernelSet"] == "client":
        command.extend([
            "--client-kernels", "--effect-root", str(args.effect_root),
            "--client-package", str(args.client_package),
        ])
    return command


def cmd_prepare(args) -> int:
    experiment, index, catalog = find_experiment(args.experiment)
    profile = args.profile or experiment["profiles"][0]
    validate_progression(experiment, index, catalog, profile)
    is_client_gpu = experiment["risk"] in ("secondary-client", "incident-equivalent")
    enrollment = enrollment_record() if is_client_gpu else None
    run_id = args.run_id or "{}-{}-{}".format(
        experiment["id"], profile.replace("-", ""), utc_now().strftime("%Y%m%dT%H%M%SZ"))
    if not re.fullmatch(r"[A-Za-z0-9_.-]+", run_id):
        raise LabError("run ID may contain only letters, digits, dot, underscore, and dash")
    run_dir = RUNS_ROOT / run_id
    if run_dir.exists():
        raise LabError("run '{}' already exists".format(run_id))
    run_dir.mkdir(parents=True, mode=0o700)
    fsync_directory(RUNS_ROOT)

    args.effect_root = Path(args.effect_root or default_effect_root()).resolve()
    args.client_package = None
    if experiment["risk"] == "secondary-client":
        args.client_package = extract_client_package(args.effect_root, run_dir / "client-package")
    command = build_worker_command(experiment, profile, args, run_dir)
    now = utc_now()
    nonce = secrets.token_urlsafe(24) if is_client_gpu else None
    authorization = None
    if nonce:
        authorization = {
            "nonce": nonce,
            "nonceSha256": sha256_bytes(nonce.encode("utf-8")),
            "createdAt": iso_time(now),
            "expiresAt": iso_time(now + dt.timedelta(hours=24)),
            "used": False,
        }
        atomic_write_json(run_dir / "authorization.json", authorization)

    artifacts = []
    binary_path = Path(command[0])
    if binary_path.is_file():
        artifacts.append(file_record(binary_path))
        dsym = Path(str(binary_path) + ".dSYM")
        if dsym.exists():
            artifacts.append({"path": str(dsym.resolve()), "kind": "dSYM"})
    resource_records = []
    if experiment.get("requiresClientResources"):
        resource_records = manifest_effect_records(args.effect_root)
    if args.manifest:
        manifest = Path(args.manifest).resolve()
        if not manifest.is_file():
            raise LabError("resource manifest is missing: {}".format(manifest))
        artifacts.append(file_record(manifest))
    if args.client_package:
        artifacts.extend(file_record(path) for path in sorted(args.client_package.parent.iterdir())
                         if path.is_file())

    ledger = {
        "schemaVersion": 1,
        "runId": run_id,
        "experimentId": experiment["id"],
        "experimentTitle": experiment["title"],
        "risk": experiment["risk"],
        "profile": profile,
        "preparedAt": iso_time(now),
        "wallEpochSeconds": now.timestamp(),
        "monotonicSeconds": time.monotonic(),
        "bootSessionUUID": boot_session_uuid(),
        "hostname": socket.gethostname(),
        "hardwareFingerprintSha256": hardware_fingerprint(),
        "repository": git_metadata(),
        "catalogSha256": sha256_file(CATALOG_PATH),
        "command": command,
        "configuration": {
            "kernelSet": experiment["kernelSet"],
            "stage": experiment["stage"],
            "dimensions": catalog["profiles"][profile]["dimensions"],
            "format": args.format,
            "dispatch": args.dispatch,
            "alias": args.alias or experiment.get("alias", "out-of-place"),
            "synchronization": args.sync,
            "temporal": args.temporal or experiment.get("temporal", "off"),
            "iterations": args.iterations,
            "validation": os.environ.get("MTL_DEBUG_LAYER", "unset"),
        },
        "authorization": {
            "required": is_client_gpu,
            "nonceSha256": authorization["nonceSha256"] if authorization else None,
            "expiresAt": authorization["expiresAt"] if authorization else None,
            "enrollmentSha256": sha256_file(ENROLLMENT_PATH) if enrollment else None,
        },
        "artifacts": artifacts,
        "clientEffectContainers": resource_records,
        "stateTransitions": [],
        "currentState": "preparing",
    }
    atomic_write_json(ledger_path(run_dir), ledger)
    transition(run_dir, "prepared")
    print("Prepared {} at {}".format(run_id, run_dir))
    if nonce:
        print("Authorization nonce expires at {}".format(authorization["expiresAt"]))
    return 0


def authorize_client_run(run_dir: Path, ledger: dict) -> str:
    enrollment_record()
    if os.environ.get("TRINITY_FROXEL_GPU_LAB") != RISK_ACK:
        raise LabError("TRINITY_FROXEL_GPU_LAB={} is required".format(RISK_ACK))
    authorization_path = run_dir / "authorization.json"
    authorization = load_json(authorization_path)
    if authorization.get("used"):
        raise LabError("the run authorization nonce has already been used")
    if parse_time(authorization["expiresAt"]) <= utc_now():
        raise LabError("the run authorization nonce expired")
    if ledger["hardwareFingerprintSha256"] != hardware_fingerprint():
        raise LabError("prepared run belongs to different hardware")
    tty = require_controlling_tty()
    try:
        phrase = "RUN {} ON {} {} WITH GPU RESET RISK".format(
            ledger["runId"], socket.gethostname(), hardware_fingerprint()[:12])
        tty.write("Client kernels may reset or lock this Mac. Type exactly:\n{}\n> ".format(phrase))
        entered = tty.readline().rstrip("\n")
        if entered != phrase:
            raise LabError("hazard confirmation did not match")
    finally:
        tty.close()
    nonce = authorization.pop("nonce")
    authorization["used"] = True
    authorization["usedAt"] = iso_time()
    atomic_write_json(authorization_path, authorization)
    return nonce


def validate_worker_authorization(run_dir: Path, ledger: dict) -> None:
    if not ledger.get("authorization", {}).get("required"):
        return
    enrollment_record()
    if os.environ.get("TRINITY_FROXEL_GPU_LAB") != RISK_ACK:
        raise LabError("client worker is missing the GPU reset-risk acknowledgement")
    nonce = os.environ.get("TRINITY_FROXEL_AUTH_NONCE", "")
    if not nonce:
        raise LabError("client worker is missing its one-use authorization nonce")
    authorization = load_json(run_dir / "authorization.json")
    if not authorization.get("used"):
        raise LabError("client authorization was not consumed by the controller")
    if parse_time(authorization["expiresAt"]) <= utc_now():
        raise LabError("client authorization expired before submission")
    if sha256_bytes(nonce.encode("utf-8")) != authorization.get("nonceSha256"):
        raise LabError("client worker authorization nonce does not match the prepared run")
    if ledger.get("hardwareFingerprintSha256") != hardware_fingerprint():
        raise LabError("client worker is running on hardware other than the prepared host")


def resolve_ledger_file(value: str) -> tuple[Path, Path, dict]:
    path = Path(value).expanduser().resolve()
    if path.name != "ledger.json" or not path.is_file():
        raise LabError("ledger path must name an existing ledger.json")
    run_dir = path.parent
    return run_dir, path, load_json(path)


def cmd_validate_authorization(args) -> int:
    run_dir, _, ledger = resolve_ledger_file(args.ledger)
    if ledger.get("currentState") != "started":
        raise LabError("authorization preflight requires a started run")
    validate_worker_authorization(run_dir, ledger)
    print("Authorization preflight passed for {}".format(ledger["runId"]))
    return 0


def cmd_mark_submitted(args) -> int:
    run_dir, _, ledger = resolve_ledger_file(args.ledger)
    if ledger.get("currentState") != "started":
        raise LabError("GPU submission marker requires a started run")
    validate_worker_authorization(run_dir, ledger)
    preflight = Path(args.preflight).expanduser().resolve()
    if not preflight.is_file() or preflight.parent != run_dir:
        raise LabError("submission preflight must be a file directly inside the run directory")
    preflight_record = file_record(preflight)
    transition(
        run_dir,
        "submitted",
        submissionMarkerSource="worker-precommit",
        workerPid=os.getppid(),
        preflight=preflight_record,
    )
    print("Marked {} submitted".format(ledger["runId"]))
    return 0


def cmd_worker(args) -> int:
    run_dir = Path(args.run_dir)
    go_path = run_dir / "worker.go"
    while not go_path.exists():
        time.sleep(0.01)
    command = list(args.command)
    if command and command[0] == "--":
        command = command[1:]
    if not command:
        return 127
    os.execvpe(command[0], command, os.environ.copy())
    return 127


def process_alive(pid: int) -> bool:
    try:
        os.kill(pid, 0)
        return True
    except OSError:
        return False


def cmd_sentinel(args) -> int:
    run_dir = Path(args.run_dir)
    pid = args.worker_pid
    logs_dir = run_dir / "sentinel"
    logs_dir.mkdir(parents=True, exist_ok=True)
    predicate = (
        'process == "WindowServer" OR process == "kernel" OR '
        'eventMessage CONTAINS[c] "AGX" OR eventMessage CONTAINS[c] "IOGPU" OR '
        'eventMessage CONTAINS[c] "Metal" OR eventMessage CONTAINS[c] "GPU"'
    )
    log_file = open(logs_dir / "focused-log-stream.ndjson", "ab", buffering=0)
    log_process = None
    try:
        log_process = subprocess.Popen(
            ["/usr/bin/log", "stream", "--style", "ndjson", "--level", "debug", "--predicate", predicate],
            stdout=log_file,
            stderr=subprocess.STDOUT,
        )
    except OSError as exc:
        atomic_write(logs_dir / "log-stream-error.txt", str(exc).encode("utf-8"))
    started = time.monotonic()
    sampled = False
    try:
        while process_alive(pid) and not (run_dir / "worker.done").exists():
            atomic_write_json(logs_dir / "heartbeat.json", {
                "wallTime": iso_time(),
                "monotonicSeconds": time.monotonic(),
                "workerPid": pid,
            })
            if not sampled and time.monotonic() - started >= 5.0:
                sampled = True
                sample_path = logs_dir / "worker-after-five-seconds.sample.txt"
                run_command([
                    "/usr/bin/sudo", "-n", "/usr/bin/sample", str(pid), "10", "1",
                    "-file", str(sample_path),
                ], timeout=20)
            time.sleep(1.0)
    finally:
        if log_process and log_process.poll() is None:
            log_process.terminate()
            try:
                log_process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                log_process.kill()
        log_file.close()
    return 0


def cmd_run(args) -> int:
    run_dir = resolve_run(args.run_id)
    ledger = load_json(ledger_path(run_dir))
    if ledger.get("currentState") != "prepared":
        raise LabError("run must be in prepared state, not {}".format(ledger.get("currentState")))
    authorization_nonce = None
    if ledger["authorization"]["required"]:
        authorization_nonce = authorize_client_run(run_dir, ledger)

    transition(run_dir, "armed", controllerPid=os.getpid())
    worker_command = [
        sys.executable, str(Path(__file__).resolve()), "_worker",
        "--run-dir", str(run_dir), "--",
    ] + ledger["command"]
    worker_environment = os.environ.copy()
    if authorization_nonce:
        worker_environment["TRINITY_FROXEL_AUTH_NONCE"] = authorization_nonce
    worker = subprocess.Popen(worker_command, env=worker_environment)
    sentinel = subprocess.Popen([
        sys.executable, str(Path(__file__).resolve()), "_sentinel",
        "--run-dir", str(run_dir), "--worker-pid", str(worker.pid),
    ])
    transition(run_dir, "started", workerPid=worker.pid, sentinelPid=sentinel.pid)
    atomic_write(run_dir / "worker.go", b"go\n")
    return_code = worker.wait()
    atomic_write(run_dir / "worker.done", (str(return_code) + "\n").encode("ascii"))
    try:
        sentinel.wait(timeout=10)
    except subprocess.TimeoutExpired:
        sentinel.terminate()
        sentinel.wait(timeout=3)

    final_ledger = load_json(ledger_path(run_dir))
    submitted_required = ledger["experimentId"] != "A00"
    if return_code == 0 and submitted_required and final_ledger.get("currentState") != "submitted":
        transition(run_dir, "failed", workerExitCode=return_code, failure="missing worker submission marker")
        raise LabError("worker exited successfully without recording a GPU submission")
    if return_code == 0:
        transition(run_dir, "completed", workerExitCode=return_code)
        print("Run {} completed".format(ledger["runId"]))
        return 0
    transition(run_dir, "failed", workerExitCode=return_code)
    raise LabError("worker exited with status {}".format(return_code))


def copy_matching_diagnostic_reports(run_dir: Path, start_epoch: float) -> list[str]:
    destination = run_dir / "collection/DiagnosticReports"
    destination.mkdir(parents=True, exist_ok=True)
    copied = []
    roots = [Path("/Library/Logs/DiagnosticReports"), Path.home() / "Library/Logs/DiagnosticReports"]
    for root in roots:
        if not root.is_dir():
            continue
        for source in root.iterdir():
            if not source.is_file() or source.suffix.lower() not in REPORT_EXTENSIONS:
                continue
            if source.stat().st_mtime < start_epoch - 300:
                continue
            target = destination / (root.name + "-" + source.name)
            shutil.copy2(source, target)
            copied.append(str(target))
    return copied


def collect_unified_logs(run_dir: Path, start: dt.datetime, end: dt.datetime) -> dict:
    destination = run_dir / "collection"
    destination.mkdir(parents=True, exist_ok=True)
    predicate = (
        'process == "WindowServer" OR process == "kernel" OR '
        'eventMessage CONTAINS[c] "AGX" OR eventMessage CONTAINS[c] "IOGPU" OR '
        'eventMessage CONTAINS[c] "Metal" OR eventMessage CONTAINS[c] "GPU"'
    )
    output = destination / "focused-unified-log.ndjson"

    def log_time(value: dt.datetime) -> str:
        return value.astimezone().strftime("%Y-%m-%d %H:%M:%S%z")

    result = run_command([
        "/usr/bin/log", "show", "--style", "ndjson",
        "--start", log_time(start), "--end", log_time(end), "--predicate", predicate,
    ], timeout=300)
    if result:
        atomic_write(output, (result.stdout + result.stderr).encode("utf-8", errors="replace"))
        return {"path": str(output), "status": result.returncode}
    return {"path": str(output), "status": "unavailable"}


def collect_system_snapshot(run_dir: Path) -> dict:
    destination = run_dir / "collection"
    destination.mkdir(parents=True, exist_ok=True)
    commands = {
        "sw-vers.txt": ["/usr/bin/sw_vers"],
        "uname.txt": ["/usr/bin/uname", "-a"],
        "uptime.txt": ["/usr/bin/uptime"],
        "last-reboot.txt": ["/usr/bin/last", "reboot"],
        "pmset-log.txt": ["/usr/bin/pmset", "-g", "log"],
        "hardware.json": ["/usr/sbin/system_profiler", "SPHardwareDataType", "SPDisplaysDataType", "-json"],
        "thermal.txt": ["/usr/bin/pmset", "-g", "therm"],
        "kexts.txt": ["/usr/bin/kmutil", "showloaded"],
    }
    statuses = {}
    for name, command in commands.items():
        result = run_command(command, timeout=120)
        payload = "command unavailable\n"
        status = "unavailable"
        if result:
            payload = result.stdout + result.stderr
            status = result.returncode
        atomic_write(destination / name, payload.encode("utf-8", errors="replace"))
        statuses[name] = status
    return statuses


def copy_symbols(run_dir: Path, ledger: dict) -> list[str]:
    destination = run_dir / "symbols"
    destination.mkdir(parents=True, exist_ok=True)
    copied = []
    for artifact in ledger.get("artifacts", []):
        source = Path(artifact["path"])
        if not source.exists():
            continue
        target = destination / source.name
        if source.is_dir():
            shutil.copytree(source, target, dirs_exist_ok=True)
        else:
            shutil.copy2(source, target)
        copied.append(str(target))
        if source.is_file():
            uuid_result = run_command(["/usr/bin/xcrun", "dwarfdump", "--uuid", str(source)])
            if uuid_result:
                atomic_write(
                    destination / (source.name + ".uuid.txt"),
                    (uuid_result.stdout + uuid_result.stderr).encode("utf-8", errors="replace"),
                )
    return copied


def cmd_collect(args) -> int:
    run_dir = resolve_run(args.run)
    ledger = load_json(ledger_path(run_dir))
    if ledger.get("currentState") == "closed":
        raise LabError("run is already closed")
    start = parse_time(ledger.get("submittedAt", ledger["preparedAt"])) - dt.timedelta(minutes=5)
    end = utc_now()
    current_boot = boot_session_uuid()
    possible_stall = ledger.get("currentState") == "submitted" or (
        ledger.get("submittedAt") and not ledger.get("completedAt"))
    collection = {
        "schemaVersion": 1,
        "collectedAt": iso_time(end),
        "runId": ledger["runId"],
        "preparedBootSessionUUID": ledger["bootSessionUUID"],
        "collectionBootSessionUUID": current_boot,
        "bootChanged": current_boot != ledger["bootSessionUUID"],
        "classification": "possible-gpu-stall" if possible_stall else ledger.get("currentState"),
        "diagnosticReports": copy_matching_diagnostic_reports(run_dir, start.timestamp()),
        "unifiedLog": collect_unified_logs(run_dir, start, end),
        "systemSnapshot": collect_system_snapshot(run_dir),
        "symbols": copy_symbols(run_dir, ledger),
        "repository": git_metadata(),
    }
    sysdiagnose_dir = run_dir / "collection/full-sysdiagnose"
    sysdiagnose_dir.mkdir(parents=True, exist_ok=True)
    if not args.skip_sysdiagnose:
        archive_name = "{}-sysdiagnose".format(ledger["runId"])
        result = run_command([
            "/usr/bin/sudo", "/usr/bin/sysdiagnose", "-f", str(sysdiagnose_dir), "-A", archive_name,
        ], timeout=1800)
        collection["sysdiagnoseStatus"] = result.returncode if result else "unavailable"
        if result:
            atomic_write(
                run_dir / "collection/sysdiagnose-command.txt",
                (result.stdout + result.stderr).encode("utf-8", errors="replace"),
            )
    else:
        collection["sysdiagnoseStatus"] = "skipped"
    atomic_write_json(run_dir / "collection/collection-index.json", collection)
    transition(run_dir, "collected", classification=collection["classification"])
    transition(run_dir, "closed")
    print("Collected and closed {} ({})".format(ledger["runId"], collection["classification"]))
    return 0


REDACTIONS = [
    (re.compile(re.escape(str(Path.home())), re.IGNORECASE), "<HOME>"),
    (re.compile(re.escape(getpass.getuser()), re.IGNORECASE), "<USER>"),
    (re.compile(r"\b[0-9A-Fa-f]{8}-[0-9A-Fa-f-]{27,}\b"), "<UUID>"),
    (re.compile(r"\b(?:\d{1,3}\.){3}\d{1,3}\b"), "<IP>"),
    (re.compile(r"\b(?:[0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}\b"), "<MAC>"),
    (re.compile(r'(?i)(serial(?: number)?["\s:=]+)[^\s",]+'), r"\1<SERIAL>"),
]


def redact_text(text: str) -> str:
    for pattern, replacement in REDACTIONS:
        text = pattern.sub(replacement, text)
    return text


def build_redacted_tree(run_dir: Path, output: Path) -> None:
    include = [ledger_path(run_dir), run_dir / "collection", run_dir / "sentinel"]
    for source in include:
        if not source.exists():
            continue
        sources = [source] if source.is_file() else [item for item in source.rglob("*") if item.is_file()]
        for item in sources:
            relative = item.relative_to(run_dir)
            target = output / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            data = item.read_bytes()
            if b"\0" in data[:4096]:
                continue
            text = data.decode("utf-8", errors="replace")
            target.write_text(redact_text(text), encoding="utf-8")


def create_tar(archive: Path, source: Path, arcname: str) -> None:
    with tarfile.open(archive, "w:gz") as tar:
        tar.add(source, arcname=arcname, recursive=True)


def cmd_bundle(args) -> int:
    run_dir = resolve_run(args.run)
    ledger = load_json(ledger_path(run_dir))
    if ledger.get("currentState") != "closed":
        raise LabError("collect and close the run before bundling")
    bundle_dir = run_dir / "bundles"
    bundle_dir.mkdir(parents=True, exist_ok=True)
    run_id = ledger["runId"]
    with tempfile.TemporaryDirectory(prefix="froxel-focused-") as temporary:
        redacted = Path(temporary) / run_id
        build_redacted_tree(run_dir, redacted)
        focused = bundle_dir / (run_id + "-focused.tar.gz")
        create_tar(focused, redacted, run_id)
    full_source = run_dir / "collection/full-sysdiagnose"
    full = bundle_dir / (run_id + "-full-sysdiagnose.tar.gz")
    create_tar(full, full_source, run_id + "-full-sysdiagnose")
    symbols_source = run_dir / "symbols"
    symbols = bundle_dir / (run_id + "-symbols.tar.gz")
    create_tar(symbols, symbols_source, run_id + "-symbols")
    archives = [focused, full, symbols]
    index = {
        "schemaVersion": 1,
        "runId": run_id,
        "createdAt": iso_time(),
        "focusedBundle": {**file_record(focused), "redacted": True},
        "fullSysdiagnoseBundle": {**file_record(full), "sensitive": True, "redacted": False},
        "symbolsBundle": {**file_record(symbols), "sensitive": True, "redacted": False},
    }
    index_path = bundle_dir / (run_id + "-bundle-index.json")
    atomic_write_json(index_path, index, mode=0o644)
    archives.append(index_path)
    sums = bundle_dir / (run_id + "-SHA256SUMS")
    lines = ["{}  {}".format(sha256_file(path), path.name) for path in archives]
    atomic_write(sums, ("\n".join(lines) + "\n").encode("ascii"), mode=0o644)
    print("Created focused, full, and symbols bundles under {}".format(bundle_dir))
    return 0


def cmd_export(args) -> int:
    run_dir = resolve_run(args.run)
    bundle_dir = run_dir / "bundles"
    if not bundle_dir.is_dir():
        raise LabError("bundle the run before export")
    destination = Path(args.destination).expanduser().resolve()
    destination.mkdir(parents=True, exist_ok=True)
    copied = []
    for source in sorted(bundle_dir.iterdir()):
        if not source.is_file():
            continue
        target = destination / source.name
        shutil.copy2(source, target)
        if sha256_file(source) != sha256_file(target):
            raise LabError("export checksum mismatch for {}".format(source.name))
        copied.append(target)
    atomic_write_json(destination / "export-verification.json", {
        "verifiedAt": iso_time(),
        "sourceRun": run_dir.name,
        "files": [file_record(path) for path in copied],
    }, mode=0o644)
    print("Exported and verified {} files to {}".format(len(copied), destination))
    return 0


def safe_extract(archive: Path, destination: Path) -> None:
    with tarfile.open(archive, "r:gz") as tar:
        root = destination.resolve()
        for member in tar.getmembers():
            if member.issym() or member.islnk():
                raise LabError("bundle contains unsupported link {}".format(member.name))
            target = (destination / member.name).resolve()
            if root not in target.parents and target != root:
                raise LabError("bundle contains unsafe path {}".format(member.name))
        tar.extractall(destination)


def cmd_analyze(args) -> int:
    bundle = Path(args.bundle).expanduser().resolve()
    if not bundle.is_file():
        raise LabError("bundle does not exist: {}".format(bundle))
    with tempfile.TemporaryDirectory(prefix="froxel-analysis-") as temporary:
        root = Path(temporary)
        safe_extract(bundle, root)
        ledgers = list(root.rglob("ledger.json"))
        if not ledgers:
            raise LabError("focused bundle contains no run ledger")
        ledger = load_json(ledgers[0])
        report_text = "\n".join(
            path.read_text(encoding="utf-8", errors="replace")
            for path in root.rglob("*")
            if path.is_file() and path.suffix.lower() in REPORT_EXTENSIONS
        )
        findings = {
            "agxMentions": len(re.findall(r"AGX", report_text, re.IGNORECASE)),
            "iogpuMentions": len(re.findall(r"IOGPU", report_text, re.IGNORECASE)),
            "watchdogMentions": len(re.findall(r"watchdog", report_text, re.IGNORECASE)),
            "windowServerMentions": len(re.findall(r"WindowServer", report_text, re.IGNORECASE)),
        }
        analysis = {
            "schemaVersion": 1,
            "analyzedAt": iso_time(),
            "bundle": file_record(bundle),
            "runId": ledger.get("runId"),
            "experimentId": ledger.get("experimentId"),
            "state": ledger.get("currentState"),
            "submittedWithoutCompletion": bool(ledger.get("submittedAt") and not ledger.get("completedAt")),
            "findings": findings,
            "rootCause": "unassigned",
        }
    output = Path(args.output).resolve() if args.output else bundle.with_name(bundle.name + "-analysis.json")
    atomic_write_json(output, analysis, mode=0o644)
    print(json.dumps(analysis, indent=2, sort_keys=True))
    return 0


def parse_cpp_layout_contract() -> dict:
    path = REPO_ROOT / "trinity/FroxelShaderLayouts.h"
    text = path.read_text(encoding="utf-8")
    size_match = re.search(r"sizeof\( PerObjectData \) == (\d+)", text)
    offsets = {
        name: int(value)
        for name, value in re.findall(
            r"offsetof\( PerObjectData, ([A-Za-z0-9_]+) \) == (\d+)", text)
    }
    if not size_match or not offsets:
        raise LabError("could not parse the C++ froxel layout contract")
    return {"size": int(size_match.group(1)), "offsets": offsets, "source": str(path)}


def disassemble_metallib(blob: bytes, temporary: Path) -> str:
    path = temporary / (sha256_bytes(blob) + ".metallib")
    path.write_bytes(blob)
    result = run_command([
        "/usr/bin/xcrun", "metal-objdump", "--metallib", "--disassemble", str(path),
    ], timeout=120)
    if not result or result.returncode != 0:
        detail = result.stderr if result else "metal-objdump unavailable"
        raise LabError("metal-objdump failed: {}".format(detail.strip()))
    return result.stdout


def inspect_air(disassembly: str) -> dict:
    argument_records = []
    metadata = {}
    for line in disassembly.splitlines():
        metadata_match = re.match(r"!(\d+) = !\{(.*)\}$", line)
        if metadata_match:
            metadata[int(metadata_match.group(1))] = metadata_match.group(2)
        if "air.arg_name" not in line:
            continue
        name = re.search(r'!"air.arg_name", !"([^"]+)"', line)
        type_name = re.search(r'!"air.arg_type_name", !"([^"]+)"', line)
        location = re.search(r'!"air.location_index", i32 (\d+)', line)
        access = re.search(r'!"air\.(read|write|read_write|sample)"', line)
        size = re.search(r'!"air.buffer_size", i32 (\d+)', line)
        argument_records.append({
            "name": name.group(1) if name else "unknown",
            "type": type_name.group(1) if type_name else "unknown",
            "location": int(location.group(1)) if location else None,
            "access": access.group(1) if access else None,
            "bufferSize": int(size.group(1)) if size else None,
        })
    fields = {}
    largest_buffer_line = None
    largest_buffer_size = 0
    for line in disassembly.splitlines():
        size_match = re.search(r'!"air.buffer_size", i32 (\d+)', line)
        if size_match and int(size_match.group(1)) > largest_buffer_size:
            largest_buffer_size = int(size_match.group(1))
            largest_buffer_line = line
    if largest_buffer_line:
        structure_match = re.search(r'!"air.struct_type_info", !(\d+)', largest_buffer_line)
        if structure_match:
            outer = metadata.get(int(structure_match.group(1)), "")
            field_table_match = re.search(r'!"air.struct_type_info", !(\d+)', outer)
            if field_table_match:
                field_table = metadata.get(int(field_table_match.group(1)), "")
                for offset, size, name in re.findall(
                        r'i32 (\d+), i32 (\d+), i32 \d+, !"[^"]+", !"([A-Za-z0-9_]+)"',
                        field_table):
                    fields[name] = {"offset": int(offset), "size": int(size)}
    return {
        "arguments": argument_records,
        "bufferSizes": sorted({item["bufferSize"] for item in argument_records if item["bufferSize"]}),
        "boundsPredicates": {
            "unsignedLessThan": len(re.findall(r"\bicmp ult\b", disassembly)),
            "unsignedLessOrEqual": len(re.findall(r"\bicmp ule\b", disassembly)),
        },
        "textureOperations": {
            "read": len(re.findall(r"@air\.read_texture", disassembly)),
            "write": len(re.findall(r"@air\.write_texture", disassembly)),
            "sample": len(re.findall(r"@air\.sample_texture", disassembly)),
        },
        "constantFields": fields,
    }


def cmd_audit(args) -> int:
    sys.path.insert(0, str(REPO_ROOT / "shadercompiler/python"))
    from shadercompiler.effectinfo import EffectInfo  # pylint: disable=import-outside-toplevel

    effect_root = Path(args.effect_root or default_effect_root()).resolve()
    containers = sorted(path for path in effect_root.glob("*.sm_*") if path.is_file())
    if not containers:
        raise LabError("no effect containers found under {}".format(effect_root))
    layout = parse_cpp_layout_contract()
    report = {
        "schemaVersion": 1,
        "auditedAt": iso_time(),
        "deviceCreated": False,
        "metalObjdumpSyntax": ["--metallib", "--disassemble"],
        "effectRoot": str(effect_root),
        "cppLayoutContract": layout,
        "containers": [],
        "errors": [],
    }
    observed_per_object_sizes = set()
    observed_offsets = {}
    with tempfile.TemporaryDirectory(prefix="froxel-air-audit-") as temporary_name:
        temporary = Path(temporary_name)
        for container in containers:
            entry = {"container": file_record(container), "version": None, "permutations": []}
            try:
                effect = EffectInfo(str(container))
                entry["version"] = effect._version
                for permutation_index in sorted(effect._offsets):
                    shader = effect.get_shader(permutation_index)
                    permutation = {"index": permutation_index, "options": [], "techniques": []}
                    if effect.permutations:
                        permutation["options"] = [
                            {"name": option.name, "value": value}
                            for option, value in effect.index_to_options(permutation_index)
                        ]
                    for technique in shader.techniques:
                        technique_record = {"name": technique.name, "passes": []}
                        for pass_index, shader_pass in enumerate(technique.passes):
                            pass_record = {"index": pass_index, "stages": []}
                            for stage_id, stage in sorted(shader_pass.stages.items()):
                                if not stage.shader_data:
                                    raise LabError("v15 stage has no retained shader blob")
                                disassembly = disassemble_metallib(stage.shader_data, temporary)
                                air = inspect_air(disassembly)
                                per_object_sizes = [size for size in air["bufferSizes"] if size >= 2000]
                                observed_per_object_sizes.update(per_object_sizes)
                                if per_object_sizes:
                                    for name, field in air["constantFields"].items():
                                        observed_offsets.setdefault(name, field["offset"])
                                pass_record["stages"].append({
                                    "stage": stage_id,
                                    "threadgroup": list(stage.thread_group_size),
                                    "pipelineSha256": sha256_bytes(stage.shader_data),
                                    "metallibBytes": len(stage.shader_data),
                                    "registers": [
                                        {
                                            "type": item.register_type,
                                            "index": item.register_index,
                                            "space": item.register_space,
                                            "arrayCount": item.array_count,
                                        }
                                        for item in stage.registers
                                    ],
                                    "resources": [vars(item) for item in stage.resources],
                                    "uavs": [vars(item) for item in stage.uavs],
                                    "air": air,
                                })
                            technique_record["passes"].append(pass_record)
                        permutation["techniques"].append(technique_record)
                    entry["permutations"].append(permutation)
            except Exception as exc:  # keep a complete audit inventory
                entry["error"] = str(exc)
                report["errors"].append({"path": str(container), "error": str(exc)})
            report["containers"].append(entry)

    offset_aliases = {
        "ResolutionX": "Resolution",
        "planets": "Planets",
    }
    layout_mismatches = []
    if observed_per_object_sizes != {layout["size"]}:
        layout_mismatches.append({
            "field": "sizeof(PerObjectData)",
            "cpu": layout["size"],
            "air": sorted(observed_per_object_sizes),
        })
    for cpu_name, cpu_offset in layout["offsets"].items():
        air_name = offset_aliases.get(cpu_name, cpu_name)
        if air_name in observed_offsets and observed_offsets[air_name] != cpu_offset:
            layout_mismatches.append({
                "field": cpu_name,
                "cpu": cpu_offset,
                "air": observed_offsets[air_name],
            })
    report["observedPerObjectBufferSizes"] = sorted(observed_per_object_sizes)
    report["layoutMismatches"] = layout_mismatches
    report["passed"] = not report["errors"] and not layout_mismatches
    output = Path(args.report).resolve()
    atomic_write_json(output, report, mode=0o644)
    print("Device-free AIR audit {}: {}".format("passed" if report["passed"] else "failed", output))
    return 0 if report["passed"] else 1


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command_name", required=True)

    inventory = subparsers.add_parser("inventory", help="record device-free host and toolchain inventory")
    inventory.add_argument("--output")
    inventory.set_defaults(func=cmd_inventory)

    enroll = subparsers.add_parser("enroll", help="enroll this physical host for sacrificial client-kernel runs")
    enroll.add_argument("--role", required=True)
    enroll.set_defaults(func=cmd_enroll)

    prepare = subparsers.add_parser("prepare", help="persist an immutable one-shot run record")
    prepare.add_argument("--experiment", required=True)
    prepare.add_argument("--run-id")
    prepare.add_argument("--profile")
    prepare.add_argument("--binary")
    prepare.add_argument("--effect-root")
    prepare.add_argument("--manifest")
    prepare.add_argument("--format", choices=("r11g11b10", "rgba16f", "rgba32f"), default="r11g11b10")
    prepare.add_argument("--temporal", choices=("off", "on"))
    prepare.add_argument("--alias", choices=("out-of-place", "in-place"))
    prepare.add_argument("--sync", choices=("none", "resource", "encoder"), default="encoder")
    prepare.add_argument("--dispatch", choices=("exact", "rounded", "reflected"), default="reflected")
    prepare.add_argument("--iterations", type=int, default=1)
    prepare.set_defaults(func=cmd_prepare)

    run = subparsers.add_parser("run", help="execute exactly one prepared experiment")
    run.add_argument("--run-id", required=True)
    run.set_defaults(func=cmd_run)

    collect = subparsers.add_parser("collect", help="collect post-run or post-reboot diagnostics")
    collect.add_argument("--run", required=True)
    collect.add_argument("--skip-sysdiagnose", action="store_true")
    collect.set_defaults(func=cmd_collect)

    bundle = subparsers.add_parser("bundle", help="create focused, full, and symbols bundles")
    bundle.add_argument("--run", required=True)
    bundle.set_defaults(func=cmd_bundle)

    export = subparsers.add_parser("export", help="copy bundles to operator-selected local storage")
    export.add_argument("--run", required=True)
    export.add_argument("--destination", required=True)
    export.set_defaults(func=cmd_export)

    analyze = subparsers.add_parser("analyze", help="analyze a returned focused diagnostic bundle")
    analyze.add_argument("--bundle", required=True)
    analyze.add_argument("--output")
    analyze.set_defaults(func=cmd_analyze)

    audit = subparsers.add_parser("audit", help="audit v15 containers and AIR without creating an MTLDevice")
    audit.add_argument("--effect-root")
    audit.add_argument("--report", required=True)
    audit.set_defaults(func=cmd_audit)

    validate_authorization = subparsers.add_parser("_validate-authorization")
    validate_authorization.add_argument("--ledger", required=True)
    validate_authorization.set_defaults(func=cmd_validate_authorization)

    mark_submitted = subparsers.add_parser("_mark-submitted")
    mark_submitted.add_argument("--ledger", required=True)
    mark_submitted.add_argument("--preflight", required=True)
    mark_submitted.set_defaults(func=cmd_mark_submitted)

    worker = subparsers.add_parser("_worker")
    worker.add_argument("--run-dir", required=True)
    worker.add_argument("command", nargs=argparse.REMAINDER)
    worker.set_defaults(func=cmd_worker)

    sentinel = subparsers.add_parser("_sentinel")
    sentinel.add_argument("--run-dir", required=True)
    sentinel.add_argument("--worker-pid", required=True, type=int)
    sentinel.set_defaults(func=cmd_sentinel)
    return parser


def main(argv=None) -> int:
    os.chdir(REPO_ROOT)
    LAB_ROOT.mkdir(parents=True, exist_ok=True)
    RUNS_ROOT.mkdir(parents=True, exist_ok=True)
    try:
        args = build_parser().parse_args(argv)
        if getattr(args, "iterations", 1) <= 0:
            raise LabError("iterations must be positive")
        return args.func(args)
    except LabError as exc:
        print("froxel_lab: {}".format(exc), file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
