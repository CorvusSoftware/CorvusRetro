#!/usr/bin/env python3
# CorvusRetro - CorvusSoftware
# Copyright (C) 2026 CorvusSoftware
# SPDX-License-Identifier: AGPL-3.0-only
"""Inspect the Android base without depending on obsolete split installers."""
import re
import shlex
import subprocess

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


