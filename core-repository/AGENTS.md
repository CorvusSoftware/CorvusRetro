# Official core repository metadata

Read the root AGENTS.md first.

- index.json (legacy packs) and cores-v2.json (individual splits) list only ACTUAL,
  released, properly signed APKs on the official
  CorvusSoftware/CorvusRetro GitHub Releases origin.
- Never invent a release download URL, hash, size, certificate or installed core.
- Run tools/validate_core_catalog.py and tools/test_core_catalog.py on every change.
- Use tools/register_core_split.py for individual cores, with the matching base APK
  and a trusted pinned signing certificate fingerprint. Keep register_core_pack.py for legacy packs.
- A format2 entry represents one signed Android core split, matched to base version
  and ABI. A legacy format1 entry represents a full CorvusRetro APK pack.
- Validate both catalogs and run tools/test_core_splits.py when changing format2 entries.
- Keep the existing JSON schema unless Android client parsing and validators
  are changed in the same reviewed PR.
- Do not add commercial ROMs, proprietary BIOS or unapproved native binaries.
