#!/usr/bin/env python3
# CorvusRetro - CorvusSoftware
# Copyright (C) 2026 CorvusSoftware
# SPDX-License-Identifier: AGPL-3.0-only
"""Validate the native archive catalog, independently of historical archive catalogs."""
import json
import re
import sys
from pathlib import Path
from urllib.parse import urlsplit

OFFICIAL_CORES = {
    "corvus.gb": ("libcorvus_gb.so", "CorvusGB"),
    "corvus.gbc": ("libcorvus_gbc.so", "CorvusGBC"),
    "corvus.gba": ("libcorvus_gba.so", "CorvusGBA"),
}
COLUMNS = {"id", "name", "description", "version", "abi", "sha256", "size",
           "archive_url", "library_name", "api_version"}
MAX_ARCHIVE = 32 * 1024 * 1024


def valid_cores(raw):
    if (not isinstance(raw, dict) or set(raw) != {"format", "cores"} or
            type(raw["format"]) is not int or raw["format"] != 3):
        raise ValueError("invalid individual core catalog format")
    cores = raw["cores"]
    if not isinstance(cores, list) or len(cores) > 32:
        raise ValueError("invalid core count")
    seen = set()
    for item in cores:
        if not isinstance(item, dict) or set(item) != COLUMNS:
            raise ValueError("invalid core fields")
        if not isinstance(item["id"], str) or item["id"] not in OFFICIAL_CORES:
            raise ValueError("unknown official core")
        if item["library_name"] != OFFICIAL_CORES[item["id"]][0]:
            raise ValueError("core and native library do not match")
        for field in ("name", "description"):
            value = item[field]
            if (not isinstance(value, str) or not 1 <= len(value) <= 128 or
                    any(ord(c) < 32 or 127 <= ord(c) <= 159 for c in value)):
                raise ValueError("invalid " + field)
        if (not isinstance(item["version"], str) or
                not re.fullmatch(r"[A-Za-z0-9.-]{1,64}", item["version"])):
            raise ValueError("invalid core version")
        if item["abi"] not in ("arm64-v8a", "x86_64"):
            raise ValueError("unknown ABI")
        if (not isinstance(item["sha256"], str) or
                not re.fullmatch(r"[a-f0-9]{64}", item["sha256"])):
            raise ValueError("invalid archive SHA-256")
        if type(item["size"]) is not int or not 0 < item["size"] <= MAX_ARCHIVE:
            raise ValueError("invalid archive size")
        if type(item["api_version"]) is not int or item["api_version"] != 0:
            raise ValueError("unsupported CorvusAPI ABI")
        address = item["archive_url"]
        if (not isinstance(address, str) or len(address) > 256 or
                any(ord(c) <= 32 or 127 <= ord(c) <= 159 or c in "\\%" for c in address)):
            raise ValueError("invalid archive URL")
        url = urlsplit(address)
        if (url.scheme != "https" or url.netloc != "github.com" or
                not url.path.startswith("/CorvusSoftware/CorvusRetro/releases/download/") or
                not url.path.endswith(".zip") or url.query or url.fragment or
                any(part in ("", ".", "..") for part in url.path.split("/")[4:])):
            raise ValueError("untrusted archive URL")
        key = (item["id"], item["abi"])
        if key in seen:
            raise ValueError("duplicate core and ABI")
        seen.add(key)
    return cores


if __name__ == "__main__":
    path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("core-repository/cores-v3.json")
    entries = valid_cores(json.loads(path.read_text(encoding="utf-8")))
    print(f"PASS individual core repository: {len(entries)} core/ABI entries")
