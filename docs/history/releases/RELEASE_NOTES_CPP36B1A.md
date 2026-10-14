# v1.2 cpp36b1a AUTO_PORTABLE_SAFE_SEED

- Adds a SHA-256 pinned ChuChu Rocket Guest ABI1 module from the previously validated online portable build.
- Confirms `dcr_game_abi_version_v1()` returns `0x00010000`.
- Confirms the current late-AOT ChuChu addresses are present in the supplied module.
- `run_game.bat --reuse` can automatically adopt the seed into an existing valid ChuChu ABI1 cache when the cache is not yet portable-safe.
- Adoption writes `.dcr_portable_guest_v1` and immediately enables normal cpp36b1 auto-portable synchronization.
- No SH-4 generation or compilation is performed during adoption.
- Adds `--no-safe-seed` as an opt-out.
