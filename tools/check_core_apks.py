#!/usr/bin/env python3
# CorvusRetro - CorvusSoftware
# Copyright (C) 2026 CorvusSoftware
# SPDX-License-Identifier: AGPL-3.0-only
"""Check a base and its three build outputs before distributing individual cores.

Debug checks compare their signing identities. Release publication additionally
requires the independently pinned certificate supplied by the caller.
"""
import argparse
import re
import subprocess
import zipfile
from pathlib import Path

from register_core_split import package_info, verify_signer
from validate_core_splits import OFFICIAL_CORES


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--variant", choices=("debug", "release"), default="debug")
    parser.add_argument("--apksigner", default="apksigner")
    parser.add_argument("--aapt", default="aapt")
    parser.add_argument("--zipalign", default="zipalign")
    parser.add_argument("--expected-cert-sha256")
    args = parser.parse_args()
    base = Path(f"app/build/outputs/apk/{args.variant}/app-{args.variant}.apk")
    result = subprocess.run([args.apksigner, "verify", "--print-certs", str(base)],
                            text=True, capture_output=True, check=True)
    certificates = re.findall(r"Signer #\d+ certificate SHA-256 digest:\s*([0-9a-fA-F]{64})",
                              result.stdout)
    if len(certificates) != 1:
        parser.error("exactly one base signing certificate is required")
    pin = args.expected_cert_sha256 or certificates[0]
    if args.variant == "release" and not args.expected_cert_sha256:
        parser.error("release checks require the independently pinned certificate")
    verify_signer(base, args.apksigner, pin.lower())
    base_info = package_info(base, args.aapt)
    if base_info.get("split"):
        parser.error("base APK is a feature split")
    with zipfile.ZipFile(base) as archive:
        libraries = {n for n in archive.namelist() if n.startswith("lib/") and n.endswith(".so")}
        expected = {f"lib/{abi}/libcorvusretro.so" for abi in ("arm64-v8a", "x86_64")}
        if libraries != expected:
            parser.error("base APK must contain only the frontend for both ABIs")
    files = [base]
    for split, library, _ in OFFICIAL_CORES.values():
        apk = Path(f"features/{split}/build/outputs/apk/{args.variant}/{split}-{args.variant}.apk")
        verify_signer(apk, args.apksigner, pin.lower())
        info = package_info(apk, args.aapt)
        if (info.get("split") != split or info["versionCode"] != base_info["versionCode"] or
                info.get("versionName") != base_info.get("versionName")):
            parser.error("feature split name or base version mismatch")
        with zipfile.ZipFile(apk) as archive:
            libraries = {n for n in archive.namelist() if n.startswith("lib/") and n.endswith(".so")}
            expected = {f"lib/{abi}/{library}" for abi in ("arm64-v8a", "x86_64")}
            if libraries != expected:
                parser.error("feature must contain only its own emulator for both ABIs")
        files.append(apk)
    for apk in files:
        subprocess.run([args.zipalign, "-c", "-P", "16", "4", str(apk)], check=True)
    print("PASS base and three independent cores: package, version, signatures, ABIs and alignment")


if __name__ == "__main__":
    main()
