# Android service instructions

Read the root AGENTS.md before editing this directory.

## Responsibilities

- Android NativeActivity host, SAF, fonts, storage, HTTPS and PackageInstaller only.
- Keep the actual CorvusRetro menus, emulation and rendering in the C11 frontend.
- Java IO runs on an executor. UI-thread activity operations use runOnUiThread.
- Event results cross into the native frontend through synchronized bounded mailboxes.
- Avoid lifecycle bugs: check destruction, cancellation and callback ownership.

## Downloaded code

- CorvusRetro is not distributed through Google Play. Android OS security still applies.
- Never dlopen a downloaded .so from writable app storage.
- An updater catalog entry describes a full APK update, not independently installable
  native cores. APK replacement can remove cores not present in the replacement.
- Restrict update URLs to CorvusSoftware official HTTPS release assets.
- Verify declared size and SHA-256. Let PackageInstaller validate package identity,
  release certificate and user confirmation. Do not bypass these system checks.
- Do not log keys, tokens or private paths. Never store release signing credentials.
- Keep CoreRepository's input validation aligned with tools/validate_core_catalog.py.

## Compatibility

- Support Android API 26 through the current target SDK.
- Do not change release signing identity during a compatible update.
- Preserve historic SharedPreferences keys and data on update.
- Run native tests, Android lint and APK packaging checks before merging.
