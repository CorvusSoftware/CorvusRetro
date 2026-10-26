# Corvus Core Repository

Copyright (C) 2026 CorvusSoftware. SPDX-License-Identifier: AGPL-3.0-only.

`cores-v3.json` is the current native core archive catalog. Each entry identifies
one official core, its CorvusAPI version, device ABI, library filename, archive URL,
exact byte size and SHA-256. The application downloads the ZIP and makes the
verified read-only native module available without Android package installation.

Build the six native modules with `./gradlew :app:assembleDebug`, then package them
with `tools/build_core_archives.py`. See [individual cores](../docs/individual-cores.md)
for the release process and runtime contract. Only real immutable public release
assets may be registered. Do not invent URLs, sizes, hashes or certificates.

`cores-v2.json` and `index.json` are preserved for the already published clients
0.11 and0.10. Their old schemas are not reinterpreted and their APK assets remain
immutable. Their obsolete pack/split publication tools have been removed.

Never distribute commercial ROMs, proprietary BIOS or unapproved native binaries.
