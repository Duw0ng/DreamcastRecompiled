# cpp37c validation

## Runtime/plugins

Linux Release compilation passed for DreamcastRuntime, dcr_network, dcr_maple and dcr_gdrom. The pre-existing optimized GCC PPP/vector warning remains unrelated.

## GD-ROM phase-3 CDDA smoke

A synthetic DiscMap V2 with one 2048-byte data track plus a raw 2352-byte audio track was exercised through the real Runtime plugin loader and cpp37c bridge:

- V3 interface discovery: PASS
- PLAY_SECTORS: PASS
- raw Red-Book PCM decode: PASS
- byte-order detection: PASS (LE)
- batched PCM bridge: PASS
- drive playhead advance: PASS
- PAUSE / RELEASE / STOP: PASS
- PLAY_TRACKS: PASS
- V3 stats: PASS

Observed summary after the 64-frame bridge change:

`PASS cpp37c phase3 cdda | frames=704 reads=2 sum=6503324`

## Project regression

Tests 1–51 and 54 passed in the normal CTest run. `generated_single_build` and `generated_program_build` were then run to completion separately and both passed. Effective regression result: 54/54 PASS.

## ABI / SAFE guest

- Guest ABI1 structural fingerprint: `00ea21b77c60d564cf2c455c9c9d870892047aea3c3a49f28bcc844b6e04054f` — unchanged.
- SAFE ChuChu guest SHA-256: `ed43078f57f3120d8510d40f024f822bbd0b6059be03ef369f8d82761ae5c1f4` — unchanged.

## Real-machine validation requested

Run ChuChu with `--reuse --gdrom-trace`. A successful cpp37c log should show `gd-native=phase3`, increasing `gd-cdda=.../pcm...` values and plugin CDDA PLAY/track-source messages while the game/music remain stable.
