# Corvus Core Repository

Copyright (C) 2026 CorvusSoftware. SPDX-License-Identifier: AGPL-3.0-only.

CorvusRetro 0.11 uses `cores-v2.json` for three independent Android core splits.
Each entry identifies its stable core, ABI and required base versionCode. Android
installs the signed module with MODE_INHERIT_EXISTING and keeps the other cores.
Use tools/register_core_split.py with the signed base APK and independently pinned
release certificate. See [individual cores](../docs/individual-cores.md).

The remaining sections describe the legacy `index.json` format used by CorvusRetro 0.10.
This is the official metadata feed used by the CorvusRetro **APK core-pack updater**.
It is not a list of executable .so files to load from app-private writable storage.

The shipped empty catalog is intentional: no signed upgradeable release APKs
have been published for this channel yet. Keep the list empty until an
APK package with the **same application ID and release signing certificate**
as the installed application has been built, tested and uploaded.

The app fetches the pinned URL:

`https://raw.githubusercontent.com/CorvusSoftware/CorvusRetro/main/core-repository/index.json`

Schema v1:

```json
{
  "format": 1,
  "packs": [{
    "id": "corvus-pack-gb",
    "name": "Corvus Core Pack",
    "description": "Official GB/GBC/GBA cores",
    "version": "1.0.0",
    "core_id": "corvus.gb",
    "abi": "arm64-v8a",
    "sha256": "<64 lowercase hex SHA-256 of the APK bytes>",
    "size": 10000000,
    "apk_url": "https://github.com/CorvusSoftware/CorvusRetro/releases/download/<tag>/<filename>.apk"
  }]
}
```

At most 32 packs; IDs, names, strings, versions, ABIs and download URLs
are validated by the Android client. The core_id is a CorvusAPI core ID.
The ABI is compared with the current device process architecture.

The file is served over HTTPS and may be edited only by trusted
CorvusSoftware maintainers. The APK bytes must match the pinned SHA-256
in this official feed. Android's PackageInstaller then validates the APK
package and signing identity, requests the user's confirmation and
refuses packages that cannot update this application. There is no
automatic execution of downloaded binary code.

**Release signing:** GitHub Actions debug APKs typically have ephemeral
signing keys; they cannot be used as a reliable updater for existing
devices. Release packs need one long-lived signing certificate securely
stored outside git and an update-compatible version code. Never publish
private signing keys. Before adding catalog entries, do on-device update,
rollback and persistence tests. The manifest hash protects against
accidental archive corruption, but is not a detached cryptographic
signature for catalog authenticity.

The user downloads a **full CorvusRetro APK with bundled cores**, not an
isolated .so. This is intentionally different from RetroArch's loose
libretro core downloader and complies with the documented execution
constraints of modern Android targets. Direct native module installation
is not exposed until independently qualified.

Do not distribute ROMs, copyrighted BIOS, or third-party executable
files without verifying the licenses and redistribution rights.
