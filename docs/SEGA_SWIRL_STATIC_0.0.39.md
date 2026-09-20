# SEGA Swirl static reconnaissance — 0.0.40

This report describes the user-supplied CDI only. No commercial-game binaries/assets are included in DreamcastRecomp.

## Disc / boot metadata

The image is an Official Sega Dreamcast Magazine January 2001 disc. Its boot metadata names `1ST_READ.BIN`, but the disc launcher configuration points the SEGA Swirl entry at `\SWIRL\SAMPLE.BIN`.

Relevant game files observed in the data filesystem include:

- `/SWIRL/0WINCEOS.BIN` — Windows CE OS image, 1,828,864 bytes
- `/SWIRL/SAMPLE.BIN` — launcher/bootstrap component, 204,576 bytes
- `/SWIRL/SWIRLDC.EXE` — game executable, 340,480 bytes
- game BMP/PVR/model/animation assets and WAV sound assets

## SWIRLDC.EXE

`SWIRLDC.EXE` is PE32 for Hitachi SH-4, Windows CE 2.12:

- PE machine: `0x01A6` (SH-4)
- image base: `0x00010000`
- entry RVA: `0x00041AE4`
- entry VA: `0x00051AE4`
- subsystem: 9 (Windows CE GUI)
- image size: `0x000A3000`
- sections: `.text`, `.rdata`, `.data`, `.pdata`

Visible imports include:

- `COREDLL.dll`
- `DDRAW.dll` / `DirectDrawCreate`
- `DINPUTX.dll` / `DirectInputCreateW`
- `DSOUND.dll`
- `MAPLEDEV.dll` / `MapleEnumerateDevices`, `MapleCreateDevice`
- `WINSOCK.dll`

## Roadmap implication

SEGA Swirl remains a strong first commercial target because its gameplay is 2D and its first useful milestone is visually obvious. However, this specific edition is not a simple raw SH-4 `1ST_READ.BIN` application. The shortest general route is to add a Windows CE/PE execution layer on top of DreamcastRecomp's hardware runtime:

```text
CDI/filesystem
  -> SAMPLE.BIN / 0WINCEOS.BIN boot context
  -> SH-4 PE loader
  -> SWIRLDC.EXE
  -> COREDLL subset
  -> DirectDraw (2D)
  -> DirectInput / MAPLEDEV
  -> DirectSound
  -> gameplay
```

Winsock/email/network behavior is not required for the first single-player playable milestone.

The project should still maintain a parallel raw-Dreamcast commercial loader because most retail titles are not Windows CE applications.
