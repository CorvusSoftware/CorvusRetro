#!/usr/bin/env python3
# CorvusRetro - CorvusSoftware
# Copyright (C) 2026 CorvusSoftware
# SPDX-License-Identifier: AGPL-3.0-only
"""Package the qualified native modules as deterministic, single-ABI archives."""
import argparse
import hashlib
import json
import re
import struct
import zipfile
from pathlib import Path
from validate_core_archives import OFFICIAL_CORES, valid_cores


def verify_elf(data, abi):
    if (len(data) < 64 or data[:6] != b"\x7fELF\x02\x01" or
            struct.unpack_from("<HH", data, 16) != (3, 183 if abi == "arm64-v8a" else 62)):
        raise ValueError("wrong ELF type or ABI")
    offset = struct.unpack_from("<Q", data, 32)[0]
    entry_size, count = struct.unpack_from("<HH", data, 54)
    if entry_size < 56 or not count or offset + entry_size * count > len(data):
        raise ValueError("invalid ELF program headers")
    loads = 0
    for index in range(count):
        position = offset + index * entry_size
        if struct.unpack_from("<I", data, position)[0] == 1:
            loads += 1
            if struct.unpack_from("<Q", data, position + 48)[0] < 16384:
                raise ValueError("native core does not support 16 KB pages")
    if not loads or b"corvus_get_api\0" not in data:
        raise ValueError("missing loadable CorvusAPI module")


def build(variant, output, tag, version):
    output.mkdir(parents=True, exist_ok=True)
    entries = []
    for core_id, (split, library, name) in OFFICIAL_CORES.items():
        apk = Path(f"features/{split}/build/outputs/apk/{variant}/{split}-{variant}.apk")
        with zipfile.ZipFile(apk) as source:
            for abi in ("arm64-v8a", "x86_64"):
                data = source.read(f"lib/{abi}/{library}")
                verify_elf(data, abi)
                archive = output / f"{name}-{version}-android-{abi}.zip"
                info = zipfile.ZipInfo(library, (2026, 1, 1, 0, 0, 0))
                info.external_attr = 0o100444 << 16
                info.compress_type = zipfile.ZIP_DEFLATED
                with zipfile.ZipFile(archive, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as target:
                    target.writestr(info, data)
                entries.append({
                    "id": core_id, "name": name,
                    "description": {"corvus.gb": "Game Boy", "corvus.gbc": "Game Boy Color",
                                    "corvus.gba": "Game Boy Advance"}[core_id],
                    "version": version, "abi": abi, "api_version": 0,
                    "library_name": library, "size": archive.stat().st_size,
                    "sha256": hashlib.sha256(archive.read_bytes()).hexdigest(),
                    "archive_url": f"https://github.com/CorvusSoftware/CorvusRetro/releases/download/{tag}/{archive.name}",
                })
    catalog = {"format": 3, "cores": entries}
    valid_cores(catalog)
    return catalog


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--variant", choices=("debug", "release"), default="debug")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--catalog", type=Path, required=True)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--version", default="0.14.0")
    args = parser.parse_args()
    if not re.fullmatch(r"v[0-9]+\.[0-9]+\.[0-9]+", args.tag):
        parser.error("invalid release tag")
    result = build(args.variant, args.output, args.tag, args.version)
    args.catalog.parent.mkdir(parents=True, exist_ok=True)
    args.catalog.write_text(json.dumps(result, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print("PASS six native core archives, CorvusAPI exports, ABIs and 16 KB segments")
