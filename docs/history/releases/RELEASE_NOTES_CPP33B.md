# v1.2 cpp33b ISP_FLASH_DETECTION

Based on the frozen v1.2 Stable via cpp33a NETWORK_BOOTSTRAP.

The ChuChu Rocket session `20261002-180246` proved that cpp33a successfully created and read persistent DreamKey ISP flash data, but the modem telemetry remained at `m0/c0/rw0,0/ppp0,0` for the entire session. cpp33b fixes the pre-modem configuration layer.

## Fixes

- Correct Katana partition-2 logical block placement.
- Logical system config block 5 is no longer placed in bitmap index 0, which Katana/KOS block scans skip.
- DreamKey C6/C7/C8 are moved to valid searchable slots.
- Existing cpp33a `dreamcast_flash.bin` is migrated automatically while preserving the console ID.
- `SYSINFO_INIT` now copies the persistent 64-bit console ID and system properties from flash into `0x8C000068`.
- Startup emits a block-map validation line (`syscfg=ok C6=ok C7=ok C8=ok`).
- Heartbeat includes flash syscall counters.

## Network stack retained from cpp33a

- Dreamcast modem aperture and status registers.
- Virtual ISP carrier path.
- PPP LCP/PAP/IPCP.
- Windows UDP/DNS bridge.
- TCP detection remains the next stage once the title reaches it.

## Validation

- Full project build: PASS.
- Regression tests 1-52 and 54: PASS.
- `generated_program_build`: PASS.
- No changes to the frozen v1.2 Stable branch.
