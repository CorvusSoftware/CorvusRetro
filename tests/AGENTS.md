# Regression tests

Read the root AGENTS.md first.

- Keep host tests synthetic, deterministic, network-free and ROM-license-safe.
- Native tests explicitly bundle the official GB/GBC/GBA modules even though the
  distributable Android base APK has no emulator cores.
- Test the zero-core UI separately, and do not assume bundled cores always exist.
- Check migration of old saved game IDs, state files and battery acknowledgements.
- Include malformed catalog/ABI input, absent cores and cancellation cases.
- Test with -Wall -Wextra -Werror and ASan/UBSan. Do not weaken tests to force green CI.
- Do not commit generated binaries or private emulator assets.
