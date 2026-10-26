#!/usr/bin/env python3
# CorvusRetro - CorvusSoftware
# Copyright (C) 2026 CorvusSoftware
# SPDX-License-Identifier: AGPL-3.0-only
"""Execute the workflows' actual Python gates without GitHub access or secrets."""
import hashlib
import json
import os
import subprocess
import sys
import tempfile
import textwrap
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPOSITORY = "CorvusSoftware/CorvusRetro"
SHA = "a" * 40


def inline_python(workflow, step):
    source = (ROOT / ".github/workflows" / workflow).read_text(encoding="utf-8")
    body = source.split("      - name: " + step + "\n", 1)[1]
    body = body.split("\n      - name:", 1)[0]
    body = body.split("python3 - <<'PY'\n", 1)[1]
    lines = body.splitlines()
    end = next(index for index, line in enumerate(lines) if line.strip() == "PY")
    return textwrap.dedent("\n".join(lines[:end]))


def signed_run(**changes):
    data = {
        "id": 41, "path": ".github/workflows/build-core-release.yml",
        "status": "completed", "conclusion": "success", "event": "workflow_run",
        "head_branch": "main", "head_repository": {"full_name": REPOSITORY},
        "head_sha": SHA,
    }
    data.update(changes)
    return data


class WorkflowGates(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.temp = Path(directory.name)
        self.env_file = self.temp / "github-env"
        self.env = {
            "PATH": os.environ.get("PATH", ""), "RUNNER_TEMP": str(self.temp),
            "GITHUB_ENV": str(self.env_file), "GITHUB_SHA": SHA,
            "SOURCE_SHA": SHA, "GITHUB_REPOSITORY": REPOSITORY,
            "REQUESTED_SOURCE_RUN": "", "CORVUS_TAG": "v0.12.0",
        }

    def run_gate(self, workflow, step):
        return subprocess.run([sys.executable, "-c", inline_python(workflow, step)],
                              cwd=ROOT, env=self.env, capture_output=True, text=True)

    def test_preview_requires_a_successful_main_push_of_the_same_source(self):
        step = "Verify the exact source passed the Android workflow"
        valid = signed_run(event="push")
        path = self.temp / "android-runs.json"
        path.write_text(json.dumps({"workflow_runs": [valid]}))
        self.assertEqual(self.run_gate("build-core-release.yml", step).returncode, 0)
        for field, value in (
                ("head_repository", {"full_name": "someone/CorvusRetro"}),
                ("head_sha", "b" * 40), ("head_branch", "feature"),
                ("event", "pull_request"), ("conclusion", "failure")):
            path.write_text(json.dumps({"workflow_runs": [dict(valid, **{field: value})]}))
            with self.subTest(field=field):
                self.assertNotEqual(self.run_gate("build-core-release.yml", step).returncode, 0)

    def test_publisher_selects_only_successful_matching_builds(self):
        step = "Select a successful signed build for the exact release commit"
        path = self.temp / "signed-runs.json"
        path.write_text(json.dumps({"workflow_runs": [signed_run(), signed_run(id=42),
                                                     signed_run(id=99, conclusion="failure")]}))
        result = self.run_gate("publish-core-archives.yml", step)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.env_file.read_text(), "SOURCE_RUN_ID=42\n")
        for field, value in (
                ("head_repository", {"full_name": "someone/CorvusRetro"}),
                ("head_sha", "b" * 40), ("head_branch", "feature"),
                ("event", "pull_request"), ("conclusion", "cancelled"),
                ("status", "in_progress"), ("path", ".github/workflows/android.yml"),
                ("id", True), ("id", "41")):
            path.write_text(json.dumps({"workflow_runs": [signed_run(**{field: value})]}))
            with self.subTest(field=field, value=value):
                self.assertNotEqual(self.run_gate("publish-core-archives.yml", step).returncode, 0)

    def test_explicit_run_does_not_fall_back_to_another_build(self):
        step = "Select a successful signed build for the exact release commit"
        self.env["REQUESTED_SOURCE_RUN"] = "41"
        path = self.temp / "signed-run.json"
        path.write_text(json.dumps(signed_run()))
        self.assertEqual(self.run_gate("publish-core-archives.yml", step).returncode, 0)
        for changed in (signed_run(id=42), signed_run(conclusion="failure")):
            path.write_text(json.dumps(changed))
            self.assertNotEqual(self.run_gate("publish-core-archives.yml", step).returncode, 0)

    def test_tag_must_resolve_to_the_qualified_commit(self):
        step = "Verify the existing tag points to the qualified source revision"
        path = self.temp / "release-tag.txt"
        for data in (SHA + "\trefs/tags/v0.12.0\n",
                     "b" * 40 + "\trefs/tags/v0.12.0\n" +
                     SHA + "\trefs/tags/v0.12.0^{}\n"):
            path.write_text(data)
            self.assertEqual(self.run_gate("publish-core-archives.yml", step).returncode, 0)
        for data in ("", "b" * 40 + "\trefs/tags/v0.12.0\n",
                     SHA + "\trefs/tags/v0.12.0\n" +
                     "b" * 40 + "\trefs/tags/v0.12.0^{}\n"):
            path.write_text(data)
            self.assertNotEqual(self.run_gate("publish-core-archives.yml", step).returncode, 0)


    def test_new_release_auto_promotion_is_version_pinned(self):
        source = (ROOT / ".github/workflows/build-core-release.yml").read_text(
            encoding="utf-8")
        self.assertIn("Publish signed CorvusRetro v0.14.0", source)
        self.assertIn("gh release view v0.14.0", source)
        self.assertIn("test \"$existing\" = \"$SOURCE_SHA\"", source)
        self.assertIn("inputs[source_run_id]=$GITHUB_RUN_ID", source)
        self.assertIn("github.event_name == 'workflow_run'", source)
        self.assertIn("contents: write", source)
        self.assertIn("actions: write", source)

    def test_publisher_exports_corvuscore_metadata_only(self):
        source = (ROOT / ".github/workflows/publish-core-archives.yml").read_text(
            encoding="utf-8")
        self.assertIn('--catalog "$RUNNER_TEMP/catalog/corvuscore.json"', source)
        self.assertIn(
            'python3 tools/validate_core_archives.py "$RUNNER_TEMP/catalog/corvuscore.json"',
            source)
        self.assertIn('path: ${{ runner.temp }}/catalog/corvuscore.json', source)
        self.assertNotIn('--catalog core-repository/', source)
        self.assertNotIn('git add core-repository/cores-v3.json', source)
        self.assertNotIn('git push origin "HEAD:refs/heads/$branch"', source)

    def test_artifact_bytes_and_source_provenance_are_checked(self):
        step = "Download only the matching signed artifact"
        folder = self.temp / "signed-core-apks"
        folder.mkdir()
        names = ["CorvusRetro-base-release.apk"] + [
            "libcorvus_" + core + "-" + abi + ".so"
            for core in ("gb", "gbc", "gba") for abi in ("arm64-v8a", "x86_64")]
        for name in names:
            (folder / name).write_bytes(b"original test fixture")
        def revision(path):
            return subprocess.check_output(["git", "-C", str(ROOT / path), "rev-parse", "HEAD"],
                                           text=True).strip()
        source = {"source_sha": SHA, "corvuscore_sha": revision("deps/CorvusCore"),
                  "corvusapi_sha": revision("deps/CorvusCore/third_party/CorvusAPI"),
                  "rcheevos_sha": revision("deps/rcheevos")}
        def checksums():
            entries = []
            for path in sorted(folder.iterdir()):
                if path.name != "SHA256SUMS":
                    entries.append(hashlib.sha256(path.read_bytes()).hexdigest() + "  " + path.name)
            (folder / "SHA256SUMS").write_text("\n".join(entries) + "\n")
        def provenance(data):
            (folder / "source.json").write_text(json.dumps(data))
            checksums()
        provenance(source)
        self.assertEqual(self.run_gate("publish-core-archives.yml", step).returncode, 0)
        original = (folder / "libcorvus_gb-arm64-v8a.so").read_bytes()
        (folder / "libcorvus_gb-arm64-v8a.so").write_bytes(b"modified fixture")
        self.assertNotEqual(self.run_gate("publish-core-archives.yml", step).returncode, 0)
        (folder / "libcorvus_gb-arm64-v8a.so").write_bytes(original)
        for key in source:
            provenance(dict(source, **{key: "b" * 40}))
            with self.subTest(key=key):
                self.assertNotEqual(self.run_gate("publish-core-archives.yml", step).returncode, 0)
        provenance(source)
        checksum_file = folder / "SHA256SUMS"
        valid_checksums = checksum_file.read_text()
        for invalid in (valid_checksums + valid_checksums.splitlines()[0] + "\n",
                        "a" * 64 + "  ../../unrelated\n", ""):
            checksum_file.write_text(invalid)
            self.assertNotEqual(self.run_gate("publish-core-archives.yml", step).returncode, 0)


if __name__ == "__main__":
    unittest.main()
