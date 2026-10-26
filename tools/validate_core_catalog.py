#!/usr/bin/env python3
# CorvusRetro - CorvusSoftware
# Copyright (C) 2026 CorvusSoftware
# SPDX-License-Identifier: AGPL-3.0-only
"""Validate the official Corvus Core Repository metadata without network access."""
import json
import re
import sys
from pathlib import Path
from urllib.parse import urlsplit

COLUMNS = ("id", "name", "description", "version", "core_id",
           "abi", "sha256", "size", "apk_url")
ALLOWED_ARCH = {"arm64-v8a", "x86_64"}


def valid_catalog(raw):
    if not isinstance(raw, dict) or set(raw) != {"format", "packs"}:
        raise ValueError("invalid root fields")
    if type(raw["format"]) is not int or raw["format"] != 1:
        raise ValueError("unsupported schema")
    packs = raw["packs"]
    if not isinstance(packs, list) or len(packs) > 32:
        raise ValueError("invalid pack count")
    seen = set()
    for p in packs:
        if not isinstance(p, dict) or set(p) != set(COLUMNS):
            raise ValueError("invalid pack fields")
        for key in COLUMNS:
            value = p[key]
            if key == "size":
                if type(value) is not int or not 0 < value <= 300 * 1024 * 1024:
                    raise ValueError("invalid APK size")
            elif not isinstance(value, str) or not value or len(value) > 256:
                raise ValueError(f"invalid {key}")
            elif key in ("name", "description") and len(value) > 128:
                raise ValueError(f"too long {key}")
        if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_.-]{0,94}", p["id"]):
            raise ValueError("invalid pack ID")
        if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_.-]{0,94}", p["core_id"]):
            raise ValueError("invalid core ID")
        if not re.fullmatch(r"[a-zA-Z0-9.\-]{1,64}", p["version"]):
            raise ValueError("invalid version")
        if p["abi"] not in ALLOWED_ARCH:
            raise ValueError("unknown ABI")
        if not re.fullmatch(r"[a-f0-9]{64}", p["sha256"]):
            raise ValueError("invalid SHA-256")
        if any(ord(c) < 32 for key in ("name", "description") for c in p[key]):
            raise ValueError("control characters")
        u = urlsplit(p["apk_url"])
        if (u.scheme != "https" or u.netloc != "github.com" or
                not u.path.startswith("/CorvusSoftware/CorvusRetro/releases/download/") or
                not u.path.lower().endswith(".apk") or u.query or u.fragment or
                any(part in ("", ".", "..") for part in u.path.split("/")[4:])):
            raise ValueError("untrusted update URL")
        if (p["id"], p["abi"]) in seen:
            raise ValueError("duplicate pack ID and ABI")
        seen.add((p["id"], p["abi"]))
    return packs


if __name__ == "__main__":
    file = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("core-repository/index.json")
    packs = valid_catalog(json.loads(file.read_text(encoding="utf-8")))
    print(f"PASS Corvus core repository: {len(packs)} pack(s)")
