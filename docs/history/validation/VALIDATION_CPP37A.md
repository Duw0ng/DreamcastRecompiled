# cpp37a validation

## ABI

- Runtime DCRuntime fingerprint: `00ea21b77c60d564cf2c455c9c9d870892047aea3c3a49f28bcc844b6e04054f`
- Frozen Guest ABI1 fingerprint: same — MATCH.
- SAFE ChuChu guest SHA-256: `ed43078f57f3120d8510d40f024f822bbd0b6059be03ef369f8d82761ae5c1f4` — unchanged.

## Runtime build

Linux Release build passed for:

- DreamcastRuntime
- dcr_network
- dcr_maple
- dcr_gdrom

The pre-existing optimized GCC PPP-vector warning remains unrelated to cpp37a.

## GD-ROM plugin smoke

Synthetic DCR_DISC_MAP_V2 with 2 sessions / 2 tracks:

- plugin ABI2 kind 4 load: PASS
- map parse: PASS
- session enumeration: PASS
- 4096-byte bulk read across 2 sectors: PASS
- high-density TOC generation: PASS
- leadout FAD: PASS

## Runtime bridge smoke

- `dc_plugins_load_default()` discovers dcr_gdrom: PASS
- `dc_gdrom_load_disc_map()` selects plugin path: PASS
- Runtime receives compatibility track snapshot: PASS
- Runtime -> plugin bulk read: PASS
- Runtime -> plugin TOC: PASS

## Launcher

- Portable launcher v0.8 cross-compiles as PE32+ x86-64 Windows GUI: PASS
- GD-ROM Trace setting persists in launcher JSON and exports `DCR_GDROM_TRACE`: PASS
- Auto-sync script includes `dcr_gdrom.dll`: PASS

## Project regression

- Tests 1–52 + 54: PASS
- generated_single_build (#52): PASS
- generated_program_build (#53): PASS
- Effective suite: 54/54 PASS

## Portable auto-sync smoke

- Existing SAFE guest + runtime/plugin fixture synchronized without SH-4 generation: PASS
- `portable/bin/plugins/dcr_gdrom.dll`: PRESENT
- `CHUCHU_ROCKET_portable_latest.zip`: CREATED
