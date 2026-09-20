# DreamcastRecomp roadmap after 0.0.53

## Immediate 0.0.54 goals

1. Validate the 0.0.53 START path on Windows and capture the first later dynamic target.
2. Model `0x8C026F2E` as a bounded indexed tail dispatcher without recursively promoting its synthetic handler/data graph.
3. Preserve the commercial invariant of 0 unknown SH-4 / `RAW_SH4=0`.
4. Use `dispatch-cache`, ARM7 host-catch-up and PVR profile telemetry to identify the next dominant performance cost after hot target lookup is removed.
5. Optimize ARM7 only from measured behavior; keep timing/audio correctness visible rather than silently dropping more work.
6. Continue fixing title/menu texture addressing, alpha and blend behavior from guest PVR state.
7. Avoid ChuChu-specific address seeds, image masks, colors or renderer shortcuts.
