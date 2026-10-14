# GD-ROM native roadmap — cpp37 complete

cpp37a, cpp37b and cpp37c now form the complete GD-ROM extraction checkpoint. With `dcr.gdrom.control.v3` available, `dcr_gdrom.dll` owns:

- DiscMap V1/V2 parsing, physical tracks and sessions
- TOC generation and logical/bulk sector reads
- command lifecycle state, request/result synchronization and multi-transfer state
- G1/GD DMA bulk transfer and GD-DMA IRQ signaling/acknowledge
- CDDA PLAY_TRACKS / PLAY_SECTORS / PAUSE / RELEASE / STOP / SEEK
- CDDA track/FAD resolution, raw 2352-byte sector I/O, byte-order analysis and Yellow-Book compatibility
- CDDA PCM decode/playhead/device statistics

DreamcastRuntime deliberately retains only the thin BIOS/Katana marshaling layer, 75 Hz scheduling/IRQ fabric, final audio-mix handoff and a compatibility fallback. These are host/core responsibilities rather than private GD-ROM device state.

## Acceptance rule

The complete cpp37 line keeps the exact same pinned Guest ABI1 `dcr_game_module.dll`, supports `--reuse`, and keeps automatic portable synchronization.

## Next native subsystem — cpp38 AICA

Move AICA private device state behind `dcr_aica.dll` while keeping the scheduler/IRQ fabric in the core. Start with register/slot/mixer ownership and host audio backend separation, then reduce the remaining ARM7 interpreter path where practical.
