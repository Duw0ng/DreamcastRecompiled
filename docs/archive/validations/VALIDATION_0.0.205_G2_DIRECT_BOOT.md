# DreamcastRecomp 0.0.205 - G2 / Direct commercial boot validation

Date: 2026-09-18

## What changed

- Added the legal Dreamcast modem aperture at physical `0x00600000-0x006007FF`.
  With no modem attached, reads return zero and writes are ignored.
- Added the G2 External Device aperture at physical `0x01000000-0x01FFFFFF`.
  With no external device attached, reads return zero and writes are ignored.
- Added generated-runner option `--direct-game-entry=ADDR` (requires `--commercial-boot`).
- Added `run_commercial_recompiled_direct_game.bat`, which enters the commercial executable at `0x8C010000` while preserving DreamcastRecomp's BIOS/GD-ROM HLE setup.

## Source / codegen validation

- `dc_raw_recomp` 0.0.205 builds successfully on Linux/GCC.
- `cpp_emitter_tests`: PASS.
- Minimal raw SH-4 program recompiles with `Known SH-4=2`, `Unknown SH-4=0`, `RAW_SH4=0`.
- The generated 0.0.205 runner compiles with the new `--direct-game-entry` parser and handoff path.

## Commercial image validation

Test image supplied as `Daytona USA.cdi`.
Its IP.BIN internally identifies the image as:

- Product: `T19724M`
- Date: `20040423`
- Title: `PIZZICATO POLKA`
- Boot: `1ST_READ.BIN`

The filename therefore does not appear to match the internal disc metadata. It is still useful as a commercial compatibility test.

### Static closure

- Reachable functions: **3177**
- Reachable instructions: **245233**
- Known SH-4: **245233**
- Unknown SH-4: **0**
- RAW_SH4: **0**
- Relocatable templates: **3**

### Runtime progression with 0.0.205 direct-game mode

The previous faults were:

1. `0xA0600004` / physical `0x00600004` (modem aperture)
2. `0xA1000400` / physical `0x01000400` (G2 external device aperture)

Both are passed by 0.0.205.

A 25-second validation run ended only because of the host test timeout (`RC=124`), with no `SH4-FAULT` and no `DreamcastRecomp ERROR`.
Observed progression included:

- 19 GD-ROM requests
- real sector reads starting at FAD 11868
- code relocation `0x8C000600 -> 0x8C1E8918`
- code relocation `0x8C000000 -> 0x8C1E8974`
- CH2 texture transfer: `src=0x0C079100 dst=0x1152C080 len=0x4000`
- CH2 TA transfers to `0x10000000`
- heartbeat remained active through 25 seconds at guest PC `0x8C1C8CA0`
- ~421 million dispatch-cache hits by the last captured heartbeat

The image had not yet produced a rendered PVR frame in this headless validation (`pvr-frames=0`, `pvr-renders=0`), so this is a compatibility/progression validation, not a claim that the game reached a visible menu.
