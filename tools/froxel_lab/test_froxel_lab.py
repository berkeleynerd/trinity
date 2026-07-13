#!/usr/bin/env python3

import importlib.util
import io
import json
import os
import pathlib
import tarfile
import tempfile
import unittest
from unittest import mock


MODULE_PATH = pathlib.Path(__file__).with_name("froxel_lab.py")
SPEC = importlib.util.spec_from_file_location("froxel_lab", MODULE_PATH)
LAB = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(LAB)


class FroxelLabTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="froxel-lab-test-")
        self.root = pathlib.Path(self.temporary.name)
        self.old_lab_root = LAB.LAB_ROOT
        self.old_runs_root = LAB.RUNS_ROOT
        self.old_enrollment = LAB.ENROLLMENT_PATH
        LAB.LAB_ROOT = self.root
        LAB.RUNS_ROOT = self.root / "runs"
        LAB.ENROLLMENT_PATH = self.root / "enrollment.json"
        LAB.RUNS_ROOT.mkdir(parents=True)

    def tearDown(self):
        LAB.LAB_ROOT = self.old_lab_root
        LAB.RUNS_ROOT = self.old_runs_root
        LAB.ENROLLMENT_PATH = self.old_enrollment
        self.temporary.cleanup()

    def test_catalog_has_expected_order_and_unique_ids(self):
        catalog = LAB.load_catalog()
        identifiers = [item["id"] for item in catalog["experiments"]]
        self.assertEqual(len(identifiers), len(set(identifiers)))
        self.assertEqual(identifiers[:2], ["A00", "S10"])
        self.assertEqual(identifiers[-2:], ["R00", "R01"])
        self.assertEqual(set(LAB.CLIENT_STAGE_SPECS), {
            "apply_vs", "apply_ps", "mie", "calculate", "filter",
            "raymarch_out", "raymarch_in",
        })

    def test_client_scene_binary_matches_froxel_lab_target_output(self):
        build = self.root / "build"
        with mock.patch.dict(os.environ, {"TRINITY_FROXEL_BUILD_DIR": str(build)}):
            self.assertEqual(
                LAB.default_binary("client-scene"),
                build / "tools/froxel_lab/Debug/TrinityALEveSceneProbeFroxelLab_metal",
            )

    def test_client_scene_command_captures_synchronized_hdr_composite(self):
        experiment = {"id": "R00", "kernelSet": "client-scene", "temporal": "off"}
        args = type("Args", (), {"binary": None})()
        run_dir = self.root / "runs" / "R00-incident-test"
        with mock.patch.object(LAB, "resolve_executable", return_value=pathlib.Path("/tmp/probe")):
            command = LAB.build_worker_command(experiment, "incident", args, run_dir)

        product_index = command.index("--render-product")
        self.assertEqual(command[product_index + 1], "hdr-composite")
        temporal_index = command.index("--froxel-temporal")
        self.assertEqual(command[temporal_index + 1], "off")
        self.assertIn("--client-kernels", command)
        self.assertEqual(command[-2:], ["--taa", "off"])

    def test_client_scene_temporal_profile_reaches_froxel_renderer(self):
        experiment = {"id": "R01", "kernelSet": "client-scene", "temporal": "on"}
        args = type("Args", (), {"binary": None})()
        run_dir = self.root / "runs" / "R01-incident-test"
        with mock.patch.object(LAB, "resolve_executable", return_value=pathlib.Path("/tmp/probe")):
            command = LAB.build_worker_command(experiment, "incident", args, run_dir)

        temporal_index = command.index("--froxel-temporal")
        self.assertEqual(command[temporal_index + 1], "on")
        self.assertEqual(command[-2:], ["--taa", "high"])

    def test_atomic_transition_is_durable_and_ordered(self):
        run_dir = LAB.RUNS_ROOT / "run"
        run_dir.mkdir()
        LAB.atomic_write_json(run_dir / "ledger.json", {
            "schemaVersion": 1,
            "stateTransitions": [],
            "currentState": "preparing",
        })
        LAB.transition(run_dir, "prepared")
        LAB.transition(run_dir, "armed", controllerPid=42)
        ledger = LAB.load_json(run_dir / "ledger.json")
        self.assertEqual(ledger["currentState"], "armed")
        self.assertEqual([item["state"] for item in ledger["stateTransitions"]],
                         ["prepared", "armed"])
        self.assertEqual(ledger["stateTransitions"][-1]["controllerPid"], 42)

    def test_latest_run_uses_preparation_time_instead_of_name(self):
        runs = [
            ("S15-incident-20260713T150000Z", "2026-07-13T15:00:00.000000Z"),
            ("C10-onethreadgroup-20260713T151204Z", "2026-07-13T15:12:04.000000Z"),
        ]
        for name, prepared_at in runs:
            run_dir = LAB.RUNS_ROOT / name
            run_dir.mkdir()
            LAB.atomic_write_json(run_dir / "ledger.json", {
                "schemaVersion": 1,
                "preparedAt": prepared_at,
                "currentState": "failed",
            })

        self.assertEqual(LAB.resolve_run("latest").name, runs[-1][0])

    def test_closed_profiles_excludes_closed_failed_runs(self):
        for name, transitions in (
                ("passed", [{"state": "completed", "workerExitCode": 0}]),
                ("failed", [{"state": "failed", "workerExitCode": 1}])):
            run_dir = LAB.RUNS_ROOT / name
            run_dir.mkdir()
            LAB.atomic_write_json(run_dir / "ledger.json", {
                "experimentId": "C10",
                "profile": name,
                "currentState": "closed",
                "stateTransitions": transitions,
            })

        self.assertEqual(LAB.closed_profiles("C10"), {"passed"})

    def test_closed_profiles_requires_incident_report_for_scene_runs(self):
        for name, report in (("legacy", None), ("contract", {"sha256": "abc"})):
            run_dir = LAB.RUNS_ROOT / name
            run_dir.mkdir()
            completed = {"state": "completed", "workerExitCode": 0}
            if report:
                completed["incidentReport"] = report
            LAB.atomic_write_json(run_dir / "ledger.json", {
                "experimentId": "R00",
                "profile": name,
                "currentState": "closed",
                "stateTransitions": [completed],
            })

        self.assertEqual(LAB.closed_profiles("R00"), {"contract"})

    def test_controlling_tty_uses_nonseekable_read_and_write_handles(self):
        class NonSeekableTTY(io.StringIO):
            def isatty(self):
                return True

            def seekable(self):
                return False

            def seek(self, *args, **kwargs):
                raise io.UnsupportedOperation("not seekable")

        reader = NonSeekableTTY("CONFIRM\n")
        writer = NonSeekableTTY()
        modes = []

        def open_tty(path, mode, **kwargs):
            self.assertEqual(path, "/dev/tty")
            modes.append(mode)
            if mode == "r+":
                raise io.UnsupportedOperation("not seekable")
            return {"r": reader, "w": writer}[mode]

        with mock.patch("builtins.open", side_effect=open_tty):
            tty = LAB.require_controlling_tty()
            self.assertTrue(tty.isatty())
            tty.write("prompt> ")
            self.assertEqual(tty.readline(), "CONFIRM\n")
            self.assertEqual(writer.getvalue(), "prompt> ")
            tty.close()

        self.assertEqual(modes, ["r", "w"])
        self.assertTrue(reader.closed)
        self.assertTrue(writer.closed)

    def test_submission_marker_uses_worker_parent_and_preflight_hash(self):
        run_dir = LAB.RUNS_ROOT / "run"
        run_dir.mkdir()
        LAB.atomic_write_json(run_dir / "ledger.json", {
            "schemaVersion": 1,
            "runId": "run",
            "authorization": {"required": False},
            "stateTransitions": [],
            "currentState": "started",
        })
        preflight = run_dir / "submission-preflight.json"
        LAB.atomic_write_json(preflight, {"passed": True})
        args = type("Args", (), {
            "ledger": str(run_dir / "ledger.json"),
            "preflight": str(preflight),
        })()
        self.assertEqual(LAB.cmd_mark_submitted(args), 0)
        ledger = LAB.load_json(run_dir / "ledger.json")
        event = ledger["stateTransitions"][-1]
        self.assertEqual(event["state"], "submitted")
        self.assertEqual(event["workerPid"], os.getppid())
        self.assertEqual(event["preflight"]["sha256"], LAB.sha256_file(preflight))

    def test_safe_extract_rejects_parent_traversal(self):
        archive = self.root / "bad.tar.gz"
        with tarfile.open(archive, "w:gz") as output:
            payload = b"unsafe"
            member = tarfile.TarInfo("../outside")
            member.size = len(payload)
            output.addfile(member, io.BytesIO(payload))
        with self.assertRaises(LAB.LabError):
            LAB.safe_extract(archive, self.root / "extract")

    def test_focused_redaction_removes_sensitive_identifiers(self):
        source = "{} user={} ip=192.0.2.5 uuid=12345678-1234-1234-1234-123456789abc".format(
            pathlib.Path.home(), os.environ.get("USER", "unknown"))
        redacted = LAB.redact_text(source)
        self.assertNotIn(str(pathlib.Path.home()), redacted)
        self.assertNotIn("192.0.2.5", redacted)
        self.assertNotIn("12345678-1234-1234-1234-123456789abc", redacted)

    def test_focused_tree_includes_incident_contract_without_binary_capture(self):
        run_dir = LAB.RUNS_ROOT / "R00-incident-test"
        capture_dir = run_dir / "capture"
        capture_dir.mkdir(parents=True)
        LAB.atomic_write_json(run_dir / "ledger.json", {"runId": run_dir.name})
        LAB.atomic_write_json(run_dir / "submission-preflight.json", {"resourcePreparationComplete": True})
        LAB.atomic_write_json(run_dir / "incident-report.json", {"passed": True})
        (capture_dir / "incident.txt").write_text("rawHash=1234\n", encoding="utf-8")
        (capture_dir / "incident.png").write_bytes(b"\x89PNG\x00binary")

        output = self.root / "focused"
        LAB.build_redacted_tree(run_dir, output)

        self.assertTrue((output / "submission-preflight.json").is_file())
        self.assertTrue((output / "incident-report.json").is_file())
        self.assertTrue((output / "capture/incident.txt").is_file())
        self.assertFalse((output / "capture/incident.png").exists())


if __name__ == "__main__":
    unittest.main()
