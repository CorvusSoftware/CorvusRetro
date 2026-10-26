# AGENTS.md

Instructions for Codex and other coding agents working in CorvusRetro.
Read this file before any modification. Use the nearest nested AGENTS.md for more focused rules.
The current source tree is authoritative; previous chats, commits and RetroArch examples are references only.

## Project

CorvusRetro 0.12 is an original C11 Android NativeActivity frontend in the CorvusSoftware ecosystem.
It draws its own interface using EGL/OpenGL ES2 and processes audio with AAudio.
It uses CorvusAPI (MIT, experimental ABI 0) to communicate with emulation cores.
CorvusCore (AGPL-3.0-only) provides the official GB, GBC and GBA engines.
CorvusRetro itself is AGPL-3.0-only.

- CorvusAPI is the independent ABI, NOT a private CorvusCore interface.
- CorvusCore is an engine provider, NOT part of the frontend UI.
- CorvusRetro owns UI, content, settings, platform callbacks and persistence.
- Java is for Android OS services: SAF, storage, preferences, fonts, networking and verified archive storage.
  Do not implement a parallel Java emulator, renderer or application menu.
- The base Android APK ships with ZERO emulation cores. This is intentional.
- Tested official engines are selected in individual feature builds, explicit legacy
  pack builds or native regression tests.
- RetroArch/libretro are architectural and UX references. Do not copy their source,
  artwork, wording or branding. Keep the CorvusSoftware identity.

Preserve surrounding style and language. C11 stays C11, C++20 stays in CorvusCore,
Java stays Java. Prefer the smallest complete change. Do not refactor unrelated code.
Read docs/core-downloader.md and docs/stable-content-identities.md when touching cores.

## Architecture and file map

- app/src/main/cpp/frontend.c, frontend.h: menus, navigation, core selection, UI state.
- app/src/main/cpp/core_registry.c, core_registry.h: discover bundled CorvusAPI modules,
  validate ABI/core metadata and resolve permanent core IDs.
- app/src/main/cpp/engine.c, engine.h: core lifecycle, frame/audio callbacks, saves and battery.
- app/src/main/cpp/native_main.c: Android NativeActivity frontend thread and event loop.
- app/src/main/cpp/android_backend.c: JNI services, native drawing and Android bridge.
- app/src/main/cpp/audio_android.c: real-time audio output and pacing.
- app/src/main/java/com/corvussoftware/corvusretro/: Android-only services.
- core-repository/cores-v3.json: native core archives, matched to CorvusAPI ABI and device architecture.
- core-repository/cores-v2.json: historical 0.11 APK splits; preserve compatibility.
- core-repository/index.json: legacy 0.10 full-APK pack metadata; keep backward compatibility.
- tools/: offline validators and release tooling.
- third_party/CorvusCore/: pinned third-party git submodule. Never edit it in this repo.
- tests/: real-engine native tests and frontend interaction regressions.

Keep a change spanning UI, Java, C ABI or persistence consistent across all affected layers.

## Threads and callbacks

- The C frontend thread exclusively owns the active core and calls CorvusAPI.
- Do not introduce waits, disk I/O, HTTP or installation sessions into rendering/audio loops.
- Audio callbacks only access their protected PCM buffer and must not allocate or block.
- JNI network and module discovery results cross to the frontend through bounded, synchronized
  mailbox records and the native looper wakeup. Never call frontend UI functions from Java IO threads.
- Keep callbacks' userdata alive through core teardown and replacement.
- On application pause, replacement or shutdown preserve and flush battery saves.
- An unsuccessful battery write must NOT destroy or replace the running core.

## Core downloader and distribution

- The base APK contains zero emulation modules. The three feature build projects
  are builders for native CorvusAPI libraries, not the user-facing installer.
- The current downloader fetches single-core, single-ABI ZIP archives from official
  HTTPS releases. Format3 cores-v3.json describes archive size/hash, native library
  name, permanent core ID, architecture and CorvusAPI version0. No base version lock.
- Verify origin, exact archive length and SHA-256, one exact ZIP entry, bounded
  extraction and ELF architecture. Write into private files/cores using a temporary
  file marked read-only before filling it, fsync and atomic rename.
- Persist the native module hash and validate it plus read-only permissions before
  discovery. Never execute cache downloads, unverified modules or external storage.
- Verified read-only private native modules load through standard dlopen on target35.
  This was tested on Android15; do not conflate execve restrictions with dlopen.
- Keep content-addressed module paths: replacing a loaded module must not overwrite
  the inode mapped by a running engine. Preserve registry indices, full metadata,
  historical save identities and battery acknowledgements during refresh.
- Java does network/archive/storage work. Publish bounded module paths to the native
  mailbox; only the frontend thread may negotiate CorvusAPI or mutate the registry.
- Download completion refreshes the core list and game library without restarting
  or launching Android PackageInstaller. Unloading destroys the engine, not its file.
- Preserve installed legacy split discovery and both previous catalogs for old clients.
  A base update may remove old splits, but must retain all games and saves.
- Release-sign the base with the existing durable signer. Verify builder APK signatures
  before extracting the six native archives. Publish actual immutable assets before
  promoting their real metadata. Never invent URLs, hashes or certificates.

## Persistence and backward compatibility

- Core identity is corvus_core_info.id, never the order of an APK's library list.
- System identity is separate from the preferred core identity.
- Historic Game Boy IDs SHA256-0, Game Boy Color SHA256-1, and GBA SHA256-2
  and their .rom, .sav, and versioned state paths MUST continue to work.
- Community game IDs must not change when the core catalog reorders.
- Avoid destructive data migrations. Preserve games and saves if a core disappears.
- Never change battery acknowledgement or atomic file replacement without tests.

## Menu changes

Like RetroArch, a new menu entry often needs a complete multi-layer change.
Before editing a menu:
1. Locate the closest working option in frontend.c and trace its action, navigation,
   native/Java platform callback, event response, state and tests.
2. Follow the existing implementation pattern; avoid one-off UI behavior.
3. Keep the catalog selection, download status and Java service synchronized.
4. Write or expand tests/native_menu_test.c and any affected native tests.
5. Check portrait/landscape, back navigation, accessibility text bounds and zero-core behavior.

Do not copy example code from RetroArch. Preserve CorvusRetro's rendering and design.

## Comments and commit messages

- Prefer meaningful symbol names to lengthy comments.
- Write short comments explaining non-obvious constraints, not the change history.
- Keep commits focused on what and why. Do not put changelogs in commit messages.
- Prefer small, reviewable PRs; do not mix unrelated refactoring with new features.
- All original source files must retain CorvusSoftware copyright and SPDX headers:
  // CorvusRetro - CorvusSoftware
  // Copyright (C) 2026 CorvusSoftware
  // SPDX-License-Identifier: AGPL-3.0-only
- The repository's CI forbids long Unicode dash punctuation. Use plain hyphens.

## Testing and release gates

Native development (all engines enabled solely for regression tests):
- make test CMAKE=<cmake-3.22-path>
- make test SANITIZE=ON CMAKE=<cmake-3.22-path>

Android base and individual cores:
- ./gradlew --no-daemon :app:assembleDebug :app:lintDebug :features:coregb:assembleDebug :features:coregbc:assembleDebug :features:coregba:assembleDebug
- Unzip the APK and verify NO libcorvus_gb.so, libcorvus_gbc.so or libcorvus_gba.so.
- Confirm menu and library function with registry_count == 0.

Native archive installation:
- Verify six archives, one official module and one ABI per ZIP, with 16 KB ELF segments.
- Test zero cores, GBA alone, then GB/GBC without losing GBA, existing games or saves.
- Exercise truncated/bad-hash/foreign-ABI/traversal archives and service recreation.
- Check immediate native list refresh and replacement while an old module is loaded.

Catalog validation:
- python3 tools/validate_core_archives.py
- python3 tools/test_core_archives.py
- python3 tools/validate_core_catalog.py
- python3 tools/test_core_catalog.py
- python3 tools/validate_core_splits.py
- python3 tools/test_core_splits.py

Always inspect GitHub Actions results for the exact commit before merging.
Differentiate host tests, compiled APK checks, emulator tests and physical-device tests.
A compiled APK does NOT prove native archive loading works on hardware.
Before a public release, require a tested, permanently release-signed base and real native archives in GitHub Releases.

## Never commit generated artifacts

Do not commit executables, binary libraries, APKs, test fixtures containing proprietary
ROMs/BIOS, generated build trees, screenshots from captures, or release signing credentials.
Stage only intended source, documentation, manifests, scripts and tests.
Check git status and changed file list before each commit.

## Scope and safety

- Never change CorvusAPI, CorvusCore or pinned submodule history from this repo.
- For a needed public ABI change, propose the contract in CorvusAPI first.
- Do not relicense CorvusCore files or remove third-party notices.
- Never bypass Android SELinux, target SDK restrictions, package signatures or
  platform installation confirmation.
- Reject unknown executable sources, unapproved dependencies and untrusted metadata.
- Investigate an actual regression instead of weakening CI, tests, warnings or sanitizers.
