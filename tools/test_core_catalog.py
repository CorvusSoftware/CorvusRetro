#!/usr/bin/env python3
# CorvusRetro - CorvusSoftware
# Copyright (C) 2026 CorvusSoftware
# SPDX-License-Identifier: AGPL-3.0-only
"""Offline core catalog acceptance and tampering cases."""
import json
import unittest
from copy import deepcopy

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

    def test_reject_duplicate_and_schema(self):
        with self.assertRaises(ValueError):
            valid_catalog({"format": 1, "packs": [valid_pack(), valid_pack()]})
        with self.assertRaises(ValueError):
            valid_catalog({"format": 2, "packs": []})


if __name__ == "__main__":
    unittest.main()
