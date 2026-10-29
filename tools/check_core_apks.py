#!/usr/bin/env python3
# CorvusRetro - CorvusSoftware
# Copyright (C) 2026 CorvusSoftware
# SPDX-License-Identifier: AGPL-3.0-only
"""Check the core-free Android base, its signer and both frontend ELF layouts.

Release publication requires an independently pinned base certificate.
"""
import argparse
import re
import subprocess
import struct
import zipfile
from pathlib import Path

from android_artifacts import package_info, verify_signer


def verify_frontend_elf(data, abi):
    machine = {"arm64-v8a": 183, "x86_64": 62}.get(abi)
    if (machine is None or len(data) < 64 or data[:6] != b"\x7fELF\x02\x01" or
            struct.unpack_from("<HH", data, 16) != (3, machine)):
        raise ValueError("incorrect frontend ELF identity")
    offset = struct.unpack_from("<Q", data, 32)[0]
    entry_size, count = struct.unpack_from("<HH", data, 54)
    if entry_size < 56 or count == 0 or offset + entry_size * count > len(data):
        raise ValueError("invalid frontend program headers")
    loads = 0
    for index in range(count):
        position = offset + index * entry_size
        if struct.unpack_from("<I", data, position)[0] != 1:
            continue
        loads += 1
        file_offset, address = struct.unpack_from("<QQ", data, position + 8)
        file_size = struct.unpack_from("<Q", data, position + 32)[0]
        alignment = struct.unpack_from("<Q", data, position + 48)[0]
        if (alignment < 16384 or alignment & (alignment - 1) or
                file_offset + file_size > len(data) or
                file_offset % alignment != address % alignment):
            raise ValueError("frontend ELF is truncated or incompatible with 16 KB pages")
    if not loads:
        raise ValueError("frontend ELF has no loadable segments")


def verify_base_libraries(apk):
    with zipfile.ZipFile(apk) as archive:
        libraries = [name for name in archive.namelist()
                     if name.startswith("lib/") and name.endswith(".so")]
        expected = {f"lib/{abi}/libcorvusretro.so" for abi in ("arm64-v8a", "x86_64")}
        if len(libraries) != len(expected) or set(libraries) != expected:
            raise ValueError("base APK must contain only the frontend for both ABIs")
        for abi in ("arm64-v8a", "x86_64"):
            verify_frontend_elf(archive.read(f"lib/{abi}/libcorvusretro.so"), abi)


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
    verify_base_libraries(base)
    subprocess.run([args.zipalign, "-c", "-P", "16", "4", str(base)], check=True)
    print("PASS signed core-free base: package, signature, frontend ABIs and 16 KB alignment")


if __name__ == "__main__":
    main()
