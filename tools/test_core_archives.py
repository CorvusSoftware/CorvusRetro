#!/usr/bin/env python3
# CorvusRetro - CorvusSoftware
# Copyright (C) 2026 CorvusSoftware
# SPDX-License-Identifier: AGPL-3.0-only
"""Native catalog trust boundaries and executable layout regressions."""
import copy
import struct
import unittest
from build_core_archives import verify_elf
from validate_core_archives import valid_cores


def fixture():
    return {"format": 3, "cores": [{
        "id": "corvus.gb", "name": "CorvusGB", "description": "Game Boy",
        "version": "0.14.0", "abi": "arm64-v8a", "api_version": 0,
        "library_name": "libcorvus_gb.so", "size": 42, "sha256": "a" * 64,
        "archive_url": "https://github.com/CorvusSoftware/CorvusRetro/releases/download/v0.12.0/CorvusGB.zip",
    }]}


class NativeArchives(unittest.TestCase):
    def test_versions_are_independent_of_the_base_apk(self):
        self.assertEqual(len(valid_cores(fixture())), 1)
        self.assertEqual(valid_cores({"format": 3, "cores": []}), [])
        changed = fixture()
        changed["cores"][0]["base_version_code"] = 13
        with self.assertRaises(ValueError):
            valid_cores(changed)

    def test_untrusted_sources_and_types_are_rejected(self):
        original = fixture()
        for field, value in (("api_version", True), ("api_version", 1), ("size", True),
                             ("size", 34 * 1024 * 1024), ("library_name", "../libcorvus_gb.so"),
                             ("id", "unofficial.core"), ("abi", "armeabi-v7a"),
                             ("archive_url", "https://example.com/core.zip"),
                             ("archive_url", original["cores"][0]["archive_url"].replace("v0.12.0", "..")),
                             ("archive_url", original["cores"][0]["archive_url"] + "?x=1"),
                             ("name", "bad\x85text")):
            changed = copy.deepcopy(original)
            changed["cores"][0][field] = value
            with self.subTest(field=field, value=value), self.assertRaises(ValueError):
                valid_cores(changed)

    def test_duplicate_architecture_cannot_shadow_a_core(self):
        changed = fixture()
        changed["cores"].append(copy.deepcopy(changed["cores"][0]))
        with self.assertRaises(ValueError):
            valid_cores(changed)
        changed["cores"][1]["abi"] = "x86_64"
        self.assertEqual(len(valid_cores(changed)), 2)

    def test_wrong_abi_and_small_pages_are_rejected(self):
        data = bytearray(256)
        data[:6] = b"\x7fELF\x02\x01"
        struct.pack_into("<HH", data, 16, 3, 183)
        struct.pack_into("<Q", data, 32, 64)
        struct.pack_into("<HH", data, 54, 56, 1)
        struct.pack_into("<I", data, 64, 1)
        struct.pack_into("<Q", data, 112, 16384)
        data[128:143] = b"corvus_get_api\0"
        verify_elf(data, "arm64-v8a")
        with self.assertRaises(ValueError):
            verify_elf(data, "x86_64")
        struct.pack_into("<Q", data, 112, 4096)
        with self.assertRaises(ValueError):
            verify_elf(data, "arm64-v8a")


if __name__ == "__main__":
    unittest.main()
