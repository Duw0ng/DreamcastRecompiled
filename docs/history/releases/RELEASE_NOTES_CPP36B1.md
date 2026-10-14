# v1.2 cpp36b1 AUTO_PORTABLE_GUEST

- Makes portable-safe commercial guest generation the default for new SH-4 recompilations.
- Adds `--external-raw-image` / `--external-raw-base` to generated runners.
- Keeps retail BOOTSTRAP bytes outside `dcr_game_module.dll`.
- Adds automatic portable folder + ZIP synchronization from the normal per-game cache.
- Adds `--portableize` for the one-time migration of legacy embedded-raw guest caches.
- `--reuse` never recompiles SH-4. Once a guest is portable-safe, `--reuse` automatically refreshes the portable using the existing guest.
- Keeps Guest ABI1 frozen and preserves cpp36b Maple/PuruPuru/modern-input behavior.
