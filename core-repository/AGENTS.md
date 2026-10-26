# Official core repository metadata

Read the root AGENTS.md first.

- cores-v3.json is the current single-core native archive feed (format3), selected by
  architecture and CorvusAPI version. It is independent of the base APK version.
- Preserve cores-v2.json (historical 0.11 Android splits) and index.json (0.10 packs).
- List only ACTUAL immutable assets on official CorvusSoftware/CorvusRetro releases.
  Never invent URLs, hashes, sizes, certificate pins or installed modules.
- Generate format3 with tools/build_core_archives.py from the verified native build outputs.
  Run tools/validate_core_archives.py and tools/test_core_archives.py. Keep client
  and Python schemas aligned and validate previous catalogs too.
- Publish real assets before promoting their catalog to main.
- Do not add commercial ROMs, proprietary BIOS or unapproved native binaries.
