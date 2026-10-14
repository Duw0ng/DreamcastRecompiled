# cpp36b1a — AUTO_PORTABLE_SAFE_SEED

This checkpoint adopts the user-supplied ChuChu Rocket online portable guest as a prevalidated portable-safe Guest ABI1 seed.

## Seed identity

- file: `safe_guests/CHUCHU_ROCKET/dcr_game_module.dll`
- SHA-256: `ed43078f57f3120d8510d40f024f822bbd0b6059be03ef369f8d82761ae5c1f4`
- size: `50019840` bytes
- PE: x86-64 DLL
- exports: `dcr_game_abi_version_v1`, `dcr_game_attach_v1`
- ABI export result: `0x00010000` (Guest ABI1)
- source: previous ChuChu online portable build
- portable provenance: that builder generated the guest with `--no-embed-raw-image`

The binary also contains the known cpp34x late-AOT guest targets used by the current ChuChu closure, including `0x8C036280`.

## Automatic adoption

When all of these are true:

1. the selected game resolves to `CHUCHU_ROCKET`;
2. the existing cache is valid Guest ABI1;
3. the cache does not already have `.dcr_portable_guest_v1`;
4. the safe-seed manifest matches title, volume, ABI and SHA-256;

`run_game.bat --reuse` copies the safe seed over the cached `build/Release/dcr_game_module.dll`, writes the portable-safe marker, rebuilds only runtime/plugins when required, and then auto-syncs the portable folder/ZIP.

It does **not** run `dc_raw_recomp` and does **not** compile generated SH-4 translation units.

Use `--no-safe-seed` only if you intentionally want to keep the current legacy guest instead.
