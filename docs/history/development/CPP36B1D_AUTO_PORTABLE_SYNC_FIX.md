# cpp36b1d — AUTO_PORTABLE_SYNC_FIX

Fixes the case where a validated ChuChu portable-safe seed existed but `GUEST_PORTABLE` remained false because `TITLE`/`VOLUME` matching in batch was too strict.

- Safe seed adoption is now handled by `tools/portable/adopt_safe_guest.py`.
- Validation uses normalized title/volume, exact game slug, Guest ABI and pinned SHA-256.
- If the current cached `dcr_game_module.dll` already has the safe hash, no file is replaced; only the portable-safe markers are written.
- A game-root `.dcr_portable_guest_v1` marker survives stale generated-source metadata.
- `--reuse` auto-syncs `<game cache>\portable\` and `<SLUG>_portable_latest.zip`.
- `--sync-portable` performs the same reuse/runtime/plugin path, creates the portable, and exits without launching the game.
- Auto-sync errors are now visible and `--sync-portable` returns failure instead of silently continuing.

No SH-4 recompile is performed by either `--reuse` or `--sync-portable`.
