# cpp37b validation

## Guest ABI / SAFE guest

- DCRuntime fingerprint: `00ea21b77c60d564cf2c455c9c9d870892047aea3c3a49f28bcc844b6e04054f`
- Frozen Guest ABI1 fingerprint: same — MATCH
- SAFE ChuChu `dcr_game_module.dll` SHA-256: `ed43078f57f3120d8510d40f024f822bbd0b6059be03ef369f8d82761ae5c1f4` — unchanged

## Runtime/plugins

Linux Release compilation passed for DreamcastRuntime, dcr_network, dcr_maple and dcr_gdrom. The existing GCC optimized PPP/vector warning remains unrelated.

## GD-ROM phase-2 smoke

A synthetic DiscMap V2 test exercised the exported ABI2 plugin directly:

- control.v1 + control.v2 query: PASS
- command request stored in plugin: PASS
- multi read: 2 sectors / 4096 bytes: PASS
- plugin direct guest-RAM bulk write: PASS
- GD-DMA IRQ bit 14 raised by plugin: PASS
- G1 DMA end acknowledgement cleared IRQ: PASS
- phase-2 stats: PASS

Observed smoke summary:

`PASS phase=2 reads=1 sectors=2 bytes=4096 cmd=1 multi=1 dma=1 irq=1 ack=1`

## Project regression

- CTest: 54/54 PASS
- generated_single_build: PASS
- generated_program_build: PASS
- controller visual/portable source test: PASS

## Portable launcher

- v0.9 cross-compiled as PE32+ Windows x86-64 GUI: PASS
- initial title: `DreamcastRecompiled Portable [cpp37b]`
- after DiscMap probe: game-specific title with `[cpp37b]`
- runtime/PVR/status title override uses the same game title, without relinking the cached guest

## Pending real-machine validation

The synthetic phase-2 DMA path is validated. A real ChuChu run should confirm `gd-native=phase2`; if the title exercises multi/G1 DMA, `gd-xfer` and verbose `TRACE DMA transfer` counters will become non-zero. CDDA is intentionally still Runtime-owned in this checkpoint.
