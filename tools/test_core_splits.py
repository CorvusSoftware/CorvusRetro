#!/usr/bin/env python3
# CorvusRetro - CorvusSoftware
# Copyright (C) 2026 CorvusSoftware
# SPDX-License-Identifier: AGPL-3.0-only
"""Individual core metadata and signed split registration regression tests."""
import json
import unittest
import zipfile
from pathlib import Path
from tempfile import TemporaryDirectory
from types import SimpleNamespace
from unittest.mock import Mock, patch

import register_core_split
from validate_core_splits import valid_cores


def entry(**changes):
    data = {
        "id": "corvus.gba", "name": "CorvusGBA", "description": "Official GBA core",
        "version": "0.14.0", "abi": "arm64-v8a", "sha256": "a" * 64, "size": 42,
        "apk_url": "https://github.com/CorvusSoftware/CorvusRetro/releases/download/v0.11.0/coregba.apk",
        "split_name": "coregba", "base_version_code": 12,
    }
    data.update(changes)
    return data


class CoreCatalogTests(unittest.TestCase):
    def test_empty_and_multiple_base_versions(self):
        self.assertEqual(valid_cores({"format": 2, "cores": []}), [])
        items = [entry(), entry(abi="x86_64"), entry(base_version_code=13)]
        self.assertEqual(valid_cores({"format": 2, "cores": items}), items)
        with self.assertRaises(ValueError):
            valid_cores({"format": 2, "cores": [entry(), entry()]})

    def test_reject_bad_metadata(self):
        replacements = {
            "id": ["../core", "corvus.unknown", {}, None],
            "split_name": ["coregb", "base", "../coregba"],
            "name": ["", "x" * 129, "core\x7f", "core\n"],
            "description": ["", "x" * 129],
            "version": ["", "0.14/0", "x" * 65, 1],
            "abi": ["armeabi-v7a", "../arm64-v8a"],
            "sha256": ["deadbeef", "A" * 64],
            "size": [True, 0, -1, 300 * 1024 * 1024 + 1],
            "base_version_code": [True, 0, -1, 2147483648, "12"],
            "apk_url": [
                "http://github.com/CorvusSoftware/CorvusRetro/releases/download/v1/gba.apk",
                "https://evil.example/gba.apk",
                "https://github.com/Other/Repo/releases/download/v1/gba.apk",
                "https://github.com/CorvusSoftware/CorvusRetro/releases/download/../gba.apk",
                "https://github.com/CorvusSoftware/CorvusRetro/releases/download/v1//gba.apk",
                entry()["apk_url"] + "?token=example",
                entry()["apk_url"] + "#fragment",
                entry()["apk_url"].replace("coregba", "%2e%2e"),
                entry()["apk_url"].replace("coregba", "core\\gba"),
                entry()["apk_url"].replace("coregba", "core\x85gba"),
            ],
        }
        for field, values in replacements.items():
            for value in values:
                with self.subTest(field=field, value=value), self.assertRaises(ValueError):
                    valid_cores({"format": 2, "cores": [entry(**{field: value})]})
        for raw in ({"format": True, "cores": []}, {"format": 1, "cores": []},
                    {"format": 2, "cores": [], "extra": 0},
                    {"format": 2, "cores": [entry()] * 33}):
            with self.subTest(raw=raw), self.assertRaises(ValueError):
                valid_cores(raw)


class RegistrationTests(unittest.TestCase):
    def setUp(self):
        temporary = TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.base = self.root / "base.apk"
        self.split = self.root / "coregba.apk"
        self.write_apk(self.base, "libcorvusretro.so")
        self.write_apk(self.split, "libcorvus_gba.so")
        self.catalog = self.root / "cores-v2.json"
        self.catalog.write_text('{"format":2,"cores":[]}', encoding="utf-8")
        self.pin = "f" * 64
        self.base_info = {"name": register_core_split.PACKAGE, "versionCode": "12",
                          "versionName": "0.11.0"}
        self.split_info = dict(self.base_info, split="coregba")
        self.args = SimpleNamespace(
            apk=self.split, base_apk=self.base, core_id="corvus.gba", abi="arm64-v8a",
            version="0.14.0", url=entry()["apk_url"], expected_cert_sha256=self.pin,
            apksigner="apksigner", aapt="aapt", catalog=self.catalog)

    def write_apk(self, path, library):
        with zipfile.ZipFile(path, "w") as archive:
            archive.writestr("AndroidManifest.xml", "original test fixture")
            for abi in ("arm64-v8a", "x86_64"):
                archive.writestr(f"lib/{abi}/{library}", "original library fixture")

    def command(self, argv, **kwargs):
        if argv[0] == "apksigner":
            return Mock(returncode=0, stdout="Signer #1 certificate SHA-256 digest: " + self.pin)
        info = self.base_info if Path(argv[-1]) == self.base else self.split_info
        fields = " ".join(f"{k}='{v}'" for k, v in info.items())
        return Mock(returncode=0, stdout="package: " + fields + "\n")

    def register(self):
        with patch.object(register_core_split.subprocess, "run", side_effect=self.command):
            register_core_split.register(self.args)

    def test_both_abis_and_older_base_are_preserved(self):
        for abi in ("arm64-v8a", "x86_64"):
            self.args.abi = abi
            self.register()
        old = json.loads(self.catalog.read_text())["cores"]
        self.assertEqual([item["abi"] for item in old], ["arm64-v8a", "x86_64"])
        self.base_info["versionCode"] = self.split_info["versionCode"] = "13"
        self.register()
        items = valid_cores(json.loads(self.catalog.read_text()))
        self.assertEqual(len(items), 3)
        self.assertEqual(items[:2], old)
        self.assertEqual(items[-1]["base_version_code"], 13)

    def test_reject_incompatible_packages_without_catalog_change(self):
        self.register()
        before = self.catalog.read_bytes()
        changes = [("split", "coregb"), ("versionCode", "13"),
                   ("versionName", "0.12.0"), ("name", "com.other.app")]
        for field, replacement in changes:
            old = self.split_info[field]
            self.split_info[field] = replacement
            with self.subTest(field=field), self.assertRaises(ValueError):
                self.register()
            self.assertEqual(self.catalog.read_bytes(), before)
            self.split_info[field] = old
        self.args.expected_cert_sha256 = "a" * 64
        with self.assertRaises(ValueError):
            self.register()
        self.assertEqual(self.catalog.read_bytes(), before)

    def test_reject_full_pack_or_wrong_native_module(self):
        self.write_apk(self.split, "libcorvus_gb.so")
        with self.assertRaises(ValueError):
            self.register()
        self.write_apk(self.split, "libcorvus_gba.so")
        with zipfile.ZipFile(self.split, "a") as archive:
            archive.writestr("lib/arm64-v8a/libcorvusretro.so", "full APK is not a core")
        with self.assertRaises(ValueError):
            self.register()


if __name__ == "__main__":
    unittest.main()
