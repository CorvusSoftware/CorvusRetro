# Android service instructions

Read the root AGENTS.md before editing this directory.

## Responsibilities

- NativeActivity host, SAF, fonts, storage and HTTPS only. Application menus stay C11.
- Java network/archive IO runs on an executor; UI operations use the Android thread.
- Native results cross bounded synchronized mailboxes. Never mutate native core state
  from Java worker threads. Keep lifecycle, cancellation and callback ownership explicit.

## Downloaded code

- Validate official HTTPS URLs, size, SHA-256, exact ZIP entry, CorvusAPI version and ELF ABI.
- Store libraries only in private files/cores. Mark temporary modules read-only before
  writing via the open descriptor; fsync and rename atomically. Validate hashes on discovery.
- Never load writable/unverified files or libraries in cache/external storage.
- Keep old content-addressed files while a core may still use them. Preserve games/saves.
- The downloader does not request APK installation permission or open PackageInstaller.
- Keep CoreRepository format3 validation aligned with tools/validate_core_archives.py.
- Preserve legacy OS-installed split discovery and users' saves; the old JSON feeds have been intentionally removed.
- Never log credentials or private paths. Do not change target35 to work around a failure.

## Compatibility

- Support Android API 26 through the current target SDK.
- Do not change release signing identity during a compatible update.
- Preserve historic SharedPreferences keys and data on update.
- Run native tests, Android lint and APK packaging checks before merging.
