#!/usr/bin/env python3

import importlib.util
import io
import json
import os
import pathlib
import tarfile
import tempfile
import unittest


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


if __name__ == "__main__":
    unittest.main()
