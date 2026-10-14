# DreamcastRecompiled v1.2 cpp33b — ISP_FLASH_DETECTION

Built from cpp33a NETWORK_BOOTSTRAP after the ChuChu Rocket log `20261002-180246` showed the network layer stayed at `m0/c0/rw0,0/ppp0,0` for the entire run.

Changes:
- Fix Katana partition-2 log layout: logical system block 5 is now stored at bitmap index 1 (index 0 is skipped by Katana/KOS readers).
- Move DreamKey C6/C7/C8 to indexes 2/3/4 and set the first-free bitmap marker to index 5.
- Automatically migrate an existing cpp33a `dreamcast_flash.bin` while preserving the 64-bit console ID.
- Implement real SYSINFO_INIT behavior: copy persistent console ID + system properties from flash to `0x8C000068`.
- Add startup validation: `syscfg=ok C6=ok C7=ok C8=ok`.
- Add flash counters to heartbeat telemetry.
- Keep cpp33a modem/PPP/UDP/DNS bridge unchanged.

Expected next milestone: ChuChu should progress from ISP validation to actual modem reads/writes. If `m`/`rw` remain zero, the next target is the title's pre-modem network gate rather than PPP.
