#!/usr/bin/env python3
# CorvusRetro - CorvusSoftware
# Copyright (C) 2026 CorvusSoftware
# SPDX-License-Identifier: AGPL-3.0-only
"""Register an official signed Android feature split against its signed base APK."""
import argparse
import hashlib
import json
import os
import re
import shlex
import subprocess
import tempfile
import zipfile
from pathlib import Path

from validate_core_splits import MAX_APK, OFFICIAL_CORES, valid_cores

PACKAGE = "com.corvussoftware.corvusretro"


def package_info(apk, aapt):
    result = subprocess.run([aapt, "dump", "badging", str(apk)], text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    if result.returncode:
        raise ValueError("cannot inspect Android package")
    line = next((s for s in result.stdout.splitlines() if s.startswith("package:")), "")
    fields = dict(part.split("=", 1) for part in shlex.split(line[len("package:"):])
                  if "=" in part)
    if fields.get("name") != PACKAGE:
        raise ValueError("incorrect Android package identity")
    code = fields.get("versionCode", "")
    if not code.isdigit() or not 0 < int(code) <= 2147483647:
        raise ValueError("invalid Android versionCode")
    return fields


def verify_signer(apk, apksigner, expected):
    result = subprocess.run([apksigner, "verify", "--print-certs", str(apk)], text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    matches = re.findall(r"Signer #\d+ certificate SHA-256 digest:\s*([0-9a-fA-F]{64})",
                         result.stdout)
    if result.returncode or len(matches) != 1 or matches[0].lower() != expected:
        raise ValueError("APK signer differs from the pinned release certificate")


def register(args):
    expected = args.expected_cert_sha256.lower()
    if not re.fullmatch(r"[0-9a-f]{64}", expected):
        raise ValueError("invalid pinned signing certificate")
    split, library, name = OFFICIAL_CORES[args.core_id]
    for apk in (args.base_apk, args.apk):
        if not apk.is_file() or not 0 < apk.stat().st_size <= MAX_APK:
            raise ValueError("APK file absent or size outside the limit")
        verify_signer(apk, args.apksigner, expected)
    base = package_info(args.base_apk, args.aapt)
    module = package_info(args.apk, args.aapt)
    if base.get("split") or module.get("split") != split:
        raise ValueError("incorrect base or feature split name")
    if (base["versionCode"] != module["versionCode"] or
            base.get("versionName") != module.get("versionName")):
        raise ValueError("base and core split versions differ")
    with zipfile.ZipFile(args.base_apk) as archive:
        names = set(archive.namelist())
        if f"lib/{args.abi}/libcorvusretro.so" not in names:
            raise ValueError("base APK does not support the requested ABI")
        if any(re.fullmatch(r"lib/[^/]+/libcorvus_(?:gb|gbc|gba)\.so", n) for n in names):
            raise ValueError("base APK unexpectedly includes an emulator")
    with zipfile.ZipFile(args.apk) as archive:
        names = set(archive.namelist())
        if f"lib/{args.abi}/{library}" not in names:
            raise ValueError("core split does not support the requested ABI")
        if any(n.startswith("lib/") and n.endswith(".so") and
               n.rsplit("/", 1)[-1] != library for n in names):
            raise ValueError("feature split contains an unexpected native library")
    digest = hashlib.sha256()
    with args.apk.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    item = {
        "id": args.core_id, "name": name,
        "description": "Cœur officiel " + name + " pour CorvusRetro",
        "version": args.version, "abi": args.abi,
        "sha256": digest.hexdigest(), "size": args.apk.stat().st_size,
        "apk_url": args.url, "split_name": split,
        "base_version_code": int(base["versionCode"]),
    }
    catalog = json.loads(args.catalog.read_text(encoding="utf-8"))
    valid_cores(catalog)
    key = (item["id"], item["abi"], item["base_version_code"])
    catalog["cores"] = [entry for entry in catalog["cores"] if
                        (entry["id"], entry["abi"], entry["base_version_code"]) != key]
    catalog["cores"].append(item)
    catalog["cores"].sort(key=lambda c: (c["base_version_code"], c["id"], c["abi"]))
    valid_cores(catalog)
    data = json.dumps(catalog, ensure_ascii=False, indent=2) + "\n"
    fd, temporary = tempfile.mkstemp(prefix=".corvus-cores-v2-", dir=args.catalog.parent)
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as target:
            target.write(data)
            target.flush()
            os.fsync(target.fileno())
        os.replace(temporary, args.catalog)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)
    print(f"Registered {item['id']} ({item['abi']}, base {item['base_version_code']})")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--apk", required=True, type=Path)
    parser.add_argument("--base-apk", required=True, type=Path)
    parser.add_argument("--core-id", required=True, choices=OFFICIAL_CORES)
    parser.add_argument("--abi", required=True, choices=("arm64-v8a", "x86_64"))
    parser.add_argument("--version", required=True, help="CorvusAPI engine version, not app version")
    parser.add_argument("--url", required=True)
    parser.add_argument("--expected-cert-sha256", required=True,
                        help="Trusted release signer from the installed app, never the candidate APK")
    parser.add_argument("--apksigner", default=os.environ.get("APKSIGNER", "apksigner"))
    parser.add_argument("--aapt", default=os.environ.get("AAPT", "aapt"))
    parser.add_argument("--catalog", type=Path, default=Path("core-repository/cores-v2.json"))
    args = parser.parse_args()
    try:
        register(args)
    except (ValueError, OSError, zipfile.BadZipFile) as error:
        parser.error(str(error))


if __name__ == "__main__":
    main()
