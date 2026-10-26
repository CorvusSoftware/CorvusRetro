#!/usr/bin/env python3
# CorvusRetro - CorvusSoftware
# Copyright (C) 2026 CorvusSoftware
# SPDX-License-Identifier: AGPL-3.0-only
"""Register a release-signed, ABI-compatible CorvusRetro core-pack APK.

The expected SHA-256 signing certificate fingerprint comes from a trusted
previous CorvusRetro installation, never from the candidate APK itself.
No keys or credentials are created or stored by this tool.
"""
import argparse
import hashlib
import json
import os
import re
import subprocess
import tempfile
import zipfile
from pathlib import Path
from validate_core_catalog import valid_catalog


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--apk", required=True, type=Path)
    ap.add_argument("--abi", choices=["arm64-v8a", "x86_64"], required=True)
    ap.add_argument("--id", required=True)
    ap.add_argument("--name", required=True)
    ap.add_argument("--description", required=True)
    ap.add_argument("--version", required=True)
    ap.add_argument("--core-id", required=True)
    ap.add_argument("--core-library", required=True,
                    help="e.g. libcorvus_gb.so, included under lib/<abi>/")
    ap.add_argument("--url", required=True,
                    help="Official CorvusSoftware GitHub release asset URL")
    ap.add_argument("--expected-cert-sha256", required=True,
                    help="Pinned 64-digit SHA-256 fingerprint of the installed release signer")
    ap.add_argument("--apksigner", default=os.environ.get("APKSIGNER", "apksigner"))
    ap.add_argument("--catalog", type=Path, default=Path("core-repository/index.json"))
    args = ap.parse_args()

    if not re.fullmatch(r"[0-9a-f]{64}", args.expected_cert_sha256.lower()):
        ap.error("invalid pinned signing certificate fingerprint")
    if not re.fullmatch(r"lib[A-Za-z0-9_-]+\.so", args.core_library):
        ap.error("invalid native core library name")
    if not args.apk.is_file():
        ap.error("APK file not found")

    with zipfile.ZipFile(args.apk) as archive:
        paths = set(archive.namelist())
        if ("AndroidManifest.xml" not in paths or
                f"lib/{args.abi}/libcorvusretro.so" not in paths or
                f"lib/{args.abi}/{args.core_library}" not in paths):
            ap.error("APK does not bundle the requested core or frontend")

    # Android, not a JSON claim, must validate an APK's real signing identity.
    process = subprocess.run(
        [args.apksigner, "verify", "--print-certs", str(args.apk)],
        text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    if process.returncode:
        ap.error("APK signature verification failed: " + process.stderr[-300:])
    match = re.search(r"Signer #1 certificate SHA-256 digest:\s*([0-9a-fA-F]{64})",
                      process.stdout)
    if not match or match.group(1).lower() != args.expected_cert_sha256.lower():
        ap.error("APK certificate differs from the trusted installed release")

    size = args.apk.stat().st_size
    digest = hashlib.sha256()
    with args.apk.open("rb") as input_file:
        for block in iter(lambda: input_file.read(1024 * 1024), b""):
            digest.update(block)
    item = {
        "id": args.id, "name": args.name, "description": args.description,
        "version": args.version, "core_id": args.core_id, "abi": args.abi,
        "sha256": digest.hexdigest(), "size": size, "apk_url": args.url,
    }
    current = json.loads(args.catalog.read_text(encoding="utf-8"))
    valid_catalog(current)
    current["packs"] = [p for p in current["packs"]
                        if not (p["id"] == args.id and p["abi"] == args.abi)]
    current["packs"].append(item)
    current["packs"].sort(key=lambda p: (p["id"], p["abi"]))
    valid_catalog(current)

    payload = json.dumps(current, ensure_ascii=False, indent=2) + "\n"
    fd, temp_name = tempfile.mkstemp(prefix=".corvus-core-index-", dir=args.catalog.parent)
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as output:
            output.write(payload)
            output.flush()
            os.fsync(output.fileno())
        os.replace(temp_name, args.catalog)
    finally:
        if os.path.exists(temp_name):
            os.unlink(temp_name)
    print(f"Registered {args.id} ({args.abi}), size={size}, sha256={digest.hexdigest()}")


if __name__ == "__main__":
    main()
