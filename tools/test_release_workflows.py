#!/usr/bin/env python3
# CorvusRetro - CorvusSoftware
# Copyright (C) 2026 CorvusSoftware
# SPDX-License-Identifier: AGPL-3.0-only
"""Execute the workflows' actual Python gates without GitHub access or secrets."""
import hashlib
import json
import os
import subprocess
import struct
import sys
import tempfile
import textwrap
import unittest
import warnings
import zipfile
from pathlib import Path

from check_core_apks import verify_base_libraries, verify_frontend_elf

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


    def test_frontend_build_and_publisher_do_not_distribute_cores(self):
        workflow = ROOT / ".github/workflows"
        publisher = (workflow / "publish-core-archives.yml").read_text(encoding="utf-8")
        preview = (workflow / "build-core-release.yml").read_text(encoding="utf-8")
        ci = (workflow / "android.yml").read_text(encoding="utf-8")
        gradle = (ROOT / "pkg/android/app/build.gradle").read_text(encoding="utf-8")
        self.assertIn("targets 'corvusretro'", gradle)
        self.assertNotIn("'corvusretro', 'corvus_gb'", gradle)
        for source in (publisher, preview, ci):
            self.assertNotIn('tools/build_core_archives.py', source)
            self.assertNotIn('native-cores/', source)
        self.assertNotIn('*.zip', publisher)
        self.assertNotIn('catalog/corvuscore.json', publisher)
        self.assertIn('gh release create "$CORVUS_TAG" "$release_base"', publisher)
        self.assertIn('sha256sum *.apk source.json', preview)
        self.assertNotIn('build/signed-preview/*.so', preview)

    def test_artifact_bytes_and_source_provenance_are_checked(self):
        step = "Download only the matching signed artifact"
        folder = self.temp / "signed-base"
        folder.mkdir()
        names = ["CorvusRetro-base-release.apk"]
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
        original = (folder / "CorvusRetro-base-release.apk").read_bytes()
        (folder / "CorvusRetro-base-release.apk").write_bytes(b"modified fixture")
        self.assertNotEqual(self.run_gate("publish-core-archives.yml", step).returncode, 0)
        (folder / "CorvusRetro-base-release.apk").write_bytes(original)
        for key in source:
            provenance(dict(source, **{key: "b" * 40}))
            with self.subTest(key=key):
                self.assertNotEqual(self.run_gate("publish-core-archives.yml", step).returncode, 0)
        provenance(source)
        extra = folder / "libcorvus_gb-arm64-v8a.so"
        extra.write_bytes(b"core must not be a qualification input")
        self.assertNotEqual(self.run_gate("publish-core-archives.yml", step).returncode, 0)
        extra.unlink()
        checksum_file = folder / "SHA256SUMS"
        valid_checksums = checksum_file.read_text()
        for invalid in (valid_checksums + valid_checksums.splitlines()[0] + "\n",
                        "a" * 64 + "  ../../unrelated\n", ""):
            checksum_file.write_text(invalid)
            self.assertNotEqual(self.run_gate("publish-core-archives.yml", step).returncode, 0)


def frontend_elf(abi="arm64-v8a"):
    data = bytearray(256)
    data[:6] = b"\x7fELF\x02\x01"
    struct.pack_into("<HH", data, 16, 3, 183 if abi == "arm64-v8a" else 62)
    struct.pack_into("<Q", data, 32, 64)
    struct.pack_into("<HH", data, 54, 56, 1)
    struct.pack_into("<I", data, 64, 1)
    struct.pack_into("<QQ", data, 96, len(data), len(data))
    struct.pack_into("<Q", data, 112, 16384)
    return data


class BaseLayout(unittest.TestCase):
    def test_wrong_abi_truncation_and_small_pages_are_rejected(self):
        original = frontend_elf()
        verify_frontend_elf(original, "arm64-v8a")
        verify_frontend_elf(frontend_elf("x86_64"), "x86_64")
        with self.assertRaises(ValueError):
            verify_frontend_elf(original, "x86_64")
        for offset, pattern, value in ((112, "<Q", 4096), (112, "<Q", 24576),
                                       (72, "<Q", 1), (96, "<Q", 257),
                                       (54, "<H", 8), (56, "<H", 0),
                                       (32, "<Q", len(original))):
            changed = bytearray(original)
            struct.pack_into(pattern, changed, offset, value)
            with self.subTest(offset=offset, value=value), self.assertRaises(ValueError):
                verify_frontend_elf(changed, "arm64-v8a")
        with self.assertRaises(ValueError):
            verify_frontend_elf(original[:63], "arm64-v8a")

    def test_base_requires_both_frontends_and_rejects_bundled_or_duplicate_cores(self):
        with tempfile.TemporaryDirectory() as directory:
            apk = Path(directory) / "base.apk"
            def write(entries):
                with warnings.catch_warnings():
                    warnings.simplefilter("ignore", UserWarning)
                    with zipfile.ZipFile(apk, "w") as target:
                        for name, data in entries:
                            target.writestr(name, data)
            libraries = [("lib/" + abi + "/libcorvusretro.so", frontend_elf(abi))
                         for abi in ("arm64-v8a", "x86_64")]
            write(libraries)
            verify_base_libraries(apk)
            for entries in (libraries[:1], libraries + [libraries[0]],
                            libraries + [("lib/arm64-v8a/libcorvus_gb.so", b"unexpected")],
                            libraries + [("lib/armeabi-v7a/libother.so", b"unexpected")]):
                write(entries)
                with self.assertRaises(ValueError):
                    verify_base_libraries(apk)
            write([(libraries[0][0], frontend_elf("x86_64")), libraries[1]])
            with self.assertRaises(ValueError):
                verify_base_libraries(apk)


if __name__ == "__main__":
    unittest.main()
