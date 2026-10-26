#!/usr/bin/env python3
# CorvusRetro - CorvusSoftware
# Copyright (C) 2026 CorvusSoftware
# SPDX-License-Identifier: AGPL-3.0-only
"""Check the core-free Android base and six native archive inputs.

Release publication requires an independently pinned base certificate.
"""
import argparse
import re
import subprocess
import zipfile
from pathlib import Path

from android_artifacts import package_info, verify_signer
from validate_core_archives import OFFICIAL_CORES
from build_core_archives import verify_elf


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--variant", choices=("debug", "release"), default="debug")
    parser.add_argument("--apksigner", default="apksigner")
    parser.add_argument("--aapt", default="aapt")
    parser.add_argument("--zipalign", default="zipalign")
    parser.add_argument("--expected-cert-sha256")
    args = parser.parse_args()
    base = Path(f"pkg/android/app/build/outputs/apk/{args.variant}/app-{args.variant}.apk")
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
    for library, _ in OFFICIAL_CORES.values():
        for abi in ("arm64-v8a", "x86_64"):
            module = Path("pkg/android/app/build/native-cores") / args.variant.capitalize() / abi / library
            verify_elf(module.read_bytes(), abi)
    subprocess.run([args.zipalign, "-c", "-P", "16", "4", str(base)], check=True)
    print("PASS signed core-free base and six native modules: package, signature, ABIs and 16 KB alignment")


if __name__ == "__main__":
    main()
