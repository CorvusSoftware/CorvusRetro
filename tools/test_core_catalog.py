#!/usr/bin/env python3
# CorvusRetro - CorvusSoftware
# Copyright (C) 2026 CorvusSoftware
# SPDX-License-Identifier: AGPL-3.0-only
"""Offline core catalog acceptance and tampering cases."""
import json
import sys
import unittest
import zipfile
from copy import deepcopy
from pathlib import Path
from tempfile import TemporaryDirectory
from unittest.mock import patch, Mock

import register_core_pack

from validate_core_catalog import valid_catalog


def valid_pack():
    return {
        "id": "corvus-core-pack", "name": "Corvus Game Boy",
        "description": "Official cores", "version": "1.0.0",
        "core_id": "corvus.gb", "abi": "arm64-v8a",
        "sha256": "a" * 64, "size": 42,
        "apk_url": "https://github.com/CorvusSoftware/CorvusRetro/releases/download/v1/corepack.apk",
    }


class CatalogTests(unittest.TestCase):
    def test_empty_official_catalog(self):
        with open("core-repository/index.json", encoding="utf-8") as f:
            self.assertEqual(valid_catalog(json.load(f)), [])

    def test_valid_release(self):
        self.assertEqual(len(valid_catalog({"format": 1, "packs": [valid_pack()]})), 1)

    def test_reject_modified_metadata(self):
        original = valid_pack()
        replacements = [
            ("apk_url", "https://evil.example.com/pack.apk"),
            ("apk_url", "https://github.com/Other/Repo/releases/download/v1/test.apk"),
            ("apk_url", "https://github.com/CorvusSoftware/CorvusRetro/releases/download/../test.apk"),
            ("sha256", "deadbeef"),
            ("core_id", "../unsafe"),
            ("abi", "armeabi-v7a"),
            ("size", 0),
            ("size", 999999999),
        ]
        for field, replacement in replacements:
            case = deepcopy(original)
            case[field] = replacement
            with self.subTest(field=field, value=replacement), self.assertRaises(ValueError):
                valid_catalog({"format": 1, "packs": [case]})

    def test_signed_apk_publication(self):
        with TemporaryDirectory() as directory:
            root = Path(directory)
            apk = root / "pack.apk"
            with zipfile.ZipFile(apk, "w") as archive:
                archive.writestr("AndroidManifest.xml", "test")
                archive.writestr("lib/arm64-v8a/libcorvusretro.so", "app")
                archive.writestr("lib/arm64-v8a/libcorvus_gb.so", "core")
            catalog = root / "index.json"
            catalog.write_text('{"format":1,"packs":[]}', encoding="utf-8")
            trusted = "f" * 64
            args = [
                "register_core_pack.py", "--apk", str(apk), "--abi", "arm64-v8a",
                "--id", "corvus-core-pack", "--name", "Corvus Game Boy",
                "--description", "Official", "--version", "1.0",
                "--core-id", "corvus.gb", "--core-library", "libcorvus_gb.so",
                "--url", valid_pack()["apk_url"], "--catalog", str(catalog),
                "--expected-cert-sha256", trusted,
            ]
            fake = Mock(returncode=0,
                        stdout="Signer #1 certificate SHA-256 digest: " + trusted,
                        stderr="")
            with patch.object(sys, "argv", args), patch.object(
                    register_core_pack.subprocess, "run", return_value=fake):
                register_core_pack.main()
            entries = valid_catalog(json.loads(catalog.read_text(encoding="utf-8")))
            self.assertEqual(len(entries), 1)
            self.assertEqual(entries[0]["size"], apk.stat().st_size)
            self.assertEqual(entries[0]["core_id"], "corvus.gb")
            with patch.object(sys, "argv", args), patch.object(
                    register_core_pack.subprocess, "run",
                    return_value=Mock(returncode=0,
                                      stdout="Signer #1 certificate SHA-256 digest: " + "a" * 64,
                                      stderr="")):
                with self.assertRaises(SystemExit):
                    register_core_pack.main()
            self.assertEqual(len(valid_catalog(json.loads(
                catalog.read_text(encoding="utf-8")))), 1)

    def test_reject_duplicate_and_schema(self):
        with self.assertRaises(ValueError):
            valid_catalog({"format": 1, "packs": [valid_pack(), valid_pack()]})
        with self.assertRaises(ValueError):
            valid_catalog({"format": 2, "packs": []})


if __name__ == "__main__":
    unittest.main()
