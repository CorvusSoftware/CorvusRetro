# Official core repository metadata

Read the root AGENTS.md first.

- index.json lists only ACTUAL, released, properly signed APKs on the official
  CorvusSoftware/CorvusRetro GitHub Releases origin.
- Never invent a release download URL, hash, size, certificate or installed core.
- Run tools/validate_core_catalog.py and tools/test_core_catalog.py on every change.
- Use tools/register_core_pack.py with a trusted pinned signing certificate fingerprint.
- A catalog entry represents a full CorvusRetro APK pack. It is not a standalone .so
  and is not an independently composable installable core.
- Keep the existing JSON schema unless Android client parsing and validators
  are changed in the same reviewed PR.
- Do not add commercial ROMs, proprietary BIOS or unapproved native binaries.
