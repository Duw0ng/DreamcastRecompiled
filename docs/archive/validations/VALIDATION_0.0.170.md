# DreamcastRecomp 0.0.170 validation

## Scope

Compatibility-only follow-up to the real Crazy Taxi 2 0.0.169 run. The previous P2 pointer-cell crash is gone; the next fault is a legal P2 read at `0xA0000000` from guest PC `0x8C00E1B0`. The recovered function performs a boot-ROM sweep used during early Katana/G1 bootstrap.

## Runtime change

- Physical `0x00000000-0x000FFFFF`: deterministic read-only boot-ROM HLE backing.
- Physical `0x00100000-0x001FFFFF`: existing synthetic BIOS-font backing unchanged.
- P0/P1/P2 aliases use the existing 29-bit physical mapper.
- Optional external `DCR_BOOT_ROM`/`dc_boot.bin`: first 1 MiB loaded when present.
- Writes to the boot-ROM aperture remain rejected.
- Heartbeat: `bios-boot=hle0/<reads>` or `bios-boot=file/<reads>`.

## Validation

- Release source build: PASS.
- CTest: 50/50 PASS.
- Generated `sh4_smoke` C++ project: configure/build PASS.
- `generated_compile_test`: exit 0.
- Direct generated-runtime probe:
  - `dc_read32(0xA0000000)` -> 0 in BIOS-less mode;
  - `dc_read32(0x80000000)` -> 0;
  - `dc_read32(0xA00FFFFC)` -> 0;
  - boot-ROM read counter -> 3.
- Crazy Taxi 2 static closure still contains `0x8C00E1A0` with all 51 instructions decoded.

## Deliberately unchanged

PVR/TA, AICA/CDDA, GD-ROM HLE command semantics, scheduler/host-sync cadence, GPR cache policy, FPU cache policy and rendering paths.
