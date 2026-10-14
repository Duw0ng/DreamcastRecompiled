# DreamcastRecomp v1.2 cpp37c — GDROM_CDDA_NATIVE

- GD-ROM native phase 3 / `dcr.gdrom.control.v3`.
- CDDA PLAY_TRACKS / PLAY_SECTORS / PAUSE / RELEASE / STOP / SEEK moved into `dcr_gdrom.dll`.
- Raw 2352-byte Red-Book sector reader and PCM decoder moved into the plugin.
- Preserves Yellow-Book descramble, sync reconstruction, impulse repair and byte-order analysis.
- 64-frame host bridge batches avoid per-sample DLL calls.
- Runtime keeps only the 75 Hz scheduler and final audio mixing bridge.
- `gd-native=phase3` and `gd-cdda=...` counters are visible in normal heartbeat logs.
- Runtime image stream stays closed when the V3 plugin is active.
- User-visible titles use `[cpp37c]`; portable launcher is v1.0.
- Guest ABI1 and the pinned SAFE ChuChu guest are unchanged.
