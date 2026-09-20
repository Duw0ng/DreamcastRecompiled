# DreamcastRecomp 0.0.207 - Daytona PRESS START milestone

For the validated Daytona USA test path, run:

    run_daytona_0.0.207.bat "C:\ruta\Daytona USA.cdi"

This snapshot reached Daytona USA `PRESS START BUTTON` and a moving 3D attract/demo sequence. See `VALIDATION_0.0.207_DAYTONA_PRESS_START.md`.

# DreamcastRecomp 0.0.205 — G2 external / direct commercial boot

This checkpoint builds on 0.0.204 and keeps the 0.0.193 performance/compatibility lineage. It adds legal Area-0 external-device handling and an optional direct commercial game-entry path for images whose runtime IP.BIN bootstrap is incompatible with an already executable `1ST_READ.BIN`. Commercial game data is never included in the release.

## 0.0.205 highlights

- **Modem aperture:** physical `0x00600000-0x006007FF` now behaves as an unattached modem aperture instead of an unmapped-memory fault (reads `0`, writes ignored).
- **G2 External Device:** physical `0x01000000-0x01FFFFFF` now follows the unattached-device baseline used by Flycast (reads `0`, writes ignored).
- **Direct commercial entry:** generated runners accept `--direct-game-entry=ADDR` together with `--commercial-boot`.
- **Convenience runner:** `run_commercial_recompiled_direct_game.bat` uses entry `0x8C010000` while keeping BIOS/GD-ROM HLE setup.
- **Static validation:** supplied commercial image closes at **3,177 functions / 245,233 known SH-4 instructions / 0 unknown / RAW_SH4=0** when the IP.BIN bootstrap is included.
- **Runtime validation:** the direct-game path passed the previous modem and G2 external faults, completed 19 real GD-ROM requests, hit code relocation and CH2 texture/TA transfers, and remained alive for the full 25-second host validation window with no SH4 fault.

### Windows test

Normal path (unchanged):

```bat
build_windows.bat
run_commercial_recompiled.bat "C:\ruta\juego.cdi"
```

Experimental direct-game path for problematic bootstrap images:

```bat
build_windows.bat
run_commercial_recompiled_direct_game.bat "C:\ruta\juego.cdi"
```

See `VALIDATION_0.0.205_G2_DIRECT_BOOT.md` for the exact validation results.

---

# DreamcastRecomp 0.0.204 — rebased from 0.0.193

This checkpoint keeps the 0.0.193 performance/compatibility lineage while extending the symbol-free SH-4 closure far enough to execute the supplied local ChuChu Rocket! image into real PVR rendering. Commercial game data is never included in the release.

## 0.0.204 highlights

- **Packed BRA selector thunks:** a proven member can expose a compact family of consecutive `BRA shared_worker` entries whose delay slots encode consecutive selectors in the same R4-R7 register. This generically recovers the runtime-selected `0x8C10A518` path without a title-address whitelist.
- **Literal-to-JSR compact callback recovery:** when PC-relative literal provenance is proven all the way to an architectural `JSR/JMP`, a strict compact callable target is no longer rejected merely because the literal pool immediately after its `RTS` looks like a dense pointer table. This generically recovers the next runtime callback `0x8C04F698`.
- **Fail-closed closure retained:** the final supplied ChuChu checkpoint is **4,763 functions / 474,565 known SH-4 instructions / 0 unknown / RAW_SH4=0**.
- **Regression:** **53/53 tests PASS**.
- **Commercial execution:** local Linux diagnostic execution passed both previous missing SH-4 targets, performed real GD-ROM reads and reached PVR/TA rendering. The first observed live checkpoint had **7 renders / 6 flips**; the same bounded run later reached **509 renders / 505 flips** with no further unresolved SH-4 target in that interval.
- **Version diagnostics:** generated session-log filenames and heartbeat `build=` now report **0.0.204** consistently.

## ChuChu Rocket! test on Windows

Use your own `ChuChu Rocket!.cdi`:

```bat
build_windows.bat
run_commercial_recompiled.bat "C:\ruta\ChuChu Rocket!.cdi"
```

The commercial recompile path extracts/prepares the local image, generates a symbol-free native C++ project, builds it and starts the normal PVR/AICA/Maple runner. The CDI and extracted commercial bytes remain under your local generated workspace and are not redistributed.

Useful live fields include `pvr-renders=`, `pvr-flips=`, `pvr-packets=`, `pvr-tri=`, `fps=`, `gd-req=`, `dispatch-fast=`, `aica-nz=` and `cdda=`.

See `VALIDATION_0.0.204_REBASED193.md` and `CHANGELOG.md` for the closure details.
