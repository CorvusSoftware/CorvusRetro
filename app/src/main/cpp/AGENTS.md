# Native frontend instructions

Applies to app/src/main/cpp. Read the repository AGENTS.md first.

## C11 ownership

- Keep menus, layout, input, active core handles and game state in C11.
- Preserve single frontend-thread ownership of emulator cores.
- Never block the draw, input or audio paths with HTTP or package installation.
- Use CorvusAPI for all core calls; never add dependencies on CorvusCore internals.
- Keep frontend.c, frontend.h, core_registry.c, JNI calls and tests consistent.

## No-core APK

- The 0.11 base APK is valid with zero emulator modules.
- cr_registry_count() == 0 must not crash the renderer, game library or downloader.
- The "Téléchargeur de cœurs" menu entry stays present with zero cores.
- Null/empty CMake module catalogs must remain valid C11 across Linux and Android.
- Do not reintroduce bundled CorvusGB/GBC/GBA into the base APK. Discover installed
  features only through the platform-provided read-only nativeLibraryDir paths.
- A registry index is not a historic console storage suffix; use the stable core ID.

## Loading, saves and thread-safety

- Validate core metadata, required ABI functions and optional groups.
- Keep core-specific and content-specific data separate from module indices.
- Retain the original GB/GBC/GBA ROM, battery and state paths for compatibility.
- Callbacks are synchronous; do not let callbacks reference moved or freed data.
- Preserve atomic battery flushing, generation acknowledgement and failure behavior.
- Use bounded strings and path buffers. Treat downloaded catalog data as untrusted.
- Keep lock acquisition order well-defined around mailbox and Android Looper state.

## Tests

- Run native frontend, menu and engine tests with the official cores explicitly
  enabled through make test. Never make the APK bundle cores just to pass a test.
- Add a zero-core native test when changing registry and menu behavior.
- Keep -Wall -Wextra -Werror; do not silence failures.
