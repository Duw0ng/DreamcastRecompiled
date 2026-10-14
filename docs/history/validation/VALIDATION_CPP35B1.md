# cpp35b1 validation

- `VERIFY_CPP35B1.py`: PASS.
- cpp34x `DCRuntime` fingerprint equals frozen Guest ABI v1: `00ea21b77c60d564cf2c455c9c9d870892047aea3c3a49f28bcc844b6e04054f`.
- Fresh modular fixture compiles against `runtime/guest_abi/v1`.
- Standalone runtime build (`DreamcastRuntime`, `dcr_network`, `dcr_maple`) leaves the already-built `dcr_game_module` and generated guest object byte-identical and with unchanged mtimes.
- Full regression suite: 54/54 PASS.
- `generated_single_build`: PASS.
- `generated_program_build`: PASS.

Windows acceptance is intentionally guarded by `TEST_CPP35B1_NO_GUEST_REBUILD.bat`: it uses `--reuse`, so missing/invalid caches cause an error rather than a surprise SH-4 rebuild.
