# cpp37c — GD-ROM CDDA Native / Phase 3

cpp37c completes the GD-ROM device extraction started in cpp37a/cpp37b. With `dcr_gdrom.dll` V3 loaded, the plugin owns media geometry/I/O, TOC/sessions, command lifecycle state, multi-transfer/G1-GD DMA and the Red-Book CDDA transport/decoder.

The new `dcr.gdrom.control.v3` owns PLAY_TRACKS, PLAY_SECTORS, PAUSE, RELEASE, STOP, SEEK, the 75 Hz drive playhead state and raw 2352-byte audio sector decoding. The existing Yellow-Book descramble/sync-repair, impulse concealment and LE/BE detection logic was moved into the plugin rather than replaced.

DreamcastRuntime remains the small host coordinator: it schedules the 75 Hz drive clock and mixes already-decoded stereo PCM into the host audio pipeline. CDDA frames cross the DLL boundary in 64-frame batches, avoiding a 44.1 kHz DLL-call path.

When phase 3 is active the Runtime no longer opens the mapped disc image for CDDA; `dcr_gdrom.dll` is the image owner. The old Runtime CDDA code remains only as a fallback for older/missing plugins.

Guest ABI1 is unchanged and the pinned ChuChu SAFE guest must remain byte-identical.
