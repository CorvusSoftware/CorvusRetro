# AGENTS.md

Instructions for Codex and other coding agents working in CorvusRetro.
Read this file before any modification. Use the nearest nested AGENTS.md for more focused rules.
The current source tree is authoritative; previous chats, commits and RetroArch examples are references only.

## Project

CorvusRetro 0.11 is an original C11 Android NativeActivity frontend in the CorvusSoftware ecosystem.
It draws its own interface using EGL/OpenGL ES2 and processes audio with AAudio.
It uses CorvusAPI (MIT, experimental ABI 0) to communicate with emulation cores.
CorvusCore (AGPL-3.0-only) provides the official GB, GBC and GBA engines.
CorvusRetro itself is AGPL-3.0-only.

- CorvusAPI is the independent ABI, NOT a private CorvusCore interface.
- CorvusCore is an engine provider, NOT part of the frontend UI.
- CorvusRetro owns UI, content, settings, platform callbacks and persistence.
- Java is for Android OS services: SAF, storage, preferences, fonts, networking, APK installer.
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
- core-repository/cores-v2.json: individual signed core splits, matched to base version and ABI.
- core-repository/index.json: legacy 0.10 full-APK pack metadata; keep backward compatibility.
- tools/: offline validators and release tooling.
- third_party/CorvusCore/: pinned third-party git submodule. Never edit it in this repo.
- tests/: real-engine native tests and frontend interaction regressions.

Keep a change spanning UI, Java, C ABI or persistence consistent across all affected layers.

## Threads and callbacks

- The C frontend thread exclusively owns the active core and calls CorvusAPI.
- Do not introduce waits, disk I/O, HTTP or installation sessions into rendering/audio loops.
- Audio callbacks only access their protected PCM buffer and must not allocate or block.
- JNI network and APK install results cross to the frontend through bounded, synchronized
  mailbox records and the native looper wakeup. Never call frontend UI functions from Java IO threads.
- Keep callbacks' userdata alive through core teardown and replacement.
- On application pause, replacement or shutdown preserve and flush battery saves.
- An unsuccessful battery write must NOT destroy or replace the running core.

## Core downloader and distribution

- Base 0.11 ships with zero emulator modules. The three dynamic feature projects
  features/coregb, coregbc and coregba each ship one CorvusAPI module for both ABIs.
- Each core APK has the same package, versionCode and certificate as its base APK.
  Android installs it with MODE_INHERIT_EXISTING, preserving other installed cores.
- Discover only modules from this app's installed splits and read-only
  ApplicationInfo.nativeLibraryDir. Never dlopen a downloaded .so from writable storage.
- An individual catalog entry describes one core APK split, never a full frontend APK.
  Legacy index.json and register_core_pack.py still describe 0.10 full application packs.
- Core IDs are permanent; installing only GBA must keep historic SHA256-2 storage paths
  even though GBA can be registry index 0. Never use a registry index as storage identity.
- Never auto-install a module. Verify official HTTPS origin, size, hash, ABI and base
  version. Android PackageInstaller verifies package, split, version and certificate
  and requests user confirmation before native code is available.
- Flush battery and suspend emulation before installation; pending Android sessions
  outlive Activity recreation, so restore the installation barrier before opening games.
- A full base update can remove splits or change the required versionCode. Do not
  claim core APKs for different base versions are interchangeable.
- Publish actual release-signed base/core APKs before promoting their metadata to main.
  All four APKs must use the same durable signer as the installed official app.
  Do not invent URLs, hashes or certificates; never commit signing credentials or binaries.
- A release certificate cannot be rotated casually; test updates on a physical Android device.
- Preserve all contents and saves when a core is absent. No destructive migration.

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

Individual core installation:
- Verify each core APK contains only its own module, with both supported ABIs.
- Check signatures, package, version and alignment with tools/check_core_apks.py.
- Test zero cores, GBA alone, then adding GB without losing GBA or saved contents.
- Exercise cancellation and Activity recreation while Android confirmation is pending.

Catalog validation:
- python3 tools/validate_core_catalog.py
- python3 tools/test_core_catalog.py
- python3 tools/validate_core_splits.py
- python3 tools/test_core_splits.py

Always inspect GitHub Actions results for the exact commit before merging.
Differentiate host tests, compiled APK checks, emulator tests and physical-device tests.
A compiled APK does NOT prove Android PackageInstaller or native loading works on hardware.
Before a public release, require a tested, permanently release-signed pack in GitHub Releases.

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
