# Validation — cpp36b1e

- Runtime and `dcr_network` / `dcr_maple` compile successfully on the Linux validation build.
- Dedicated external-bootstrap smoke loads a 4096-byte fixture through `DCR_EXTERNAL_RAW_IMAGE` at `0x8C008000` and verifies byte-for-byte RAM contents.
- Runtime reports `external raw image loaded by Runtime ENV`.
- Portable launcher v0.5 cross-compiles to Windows x64 PE32+ and uses ENV instead of the runner CLI for external raw loading.
- Guest ABI v1 headers are unchanged.
- SAFE ChuChu `dcr_game_module.dll` remains byte-identical to the user-supplied module.
