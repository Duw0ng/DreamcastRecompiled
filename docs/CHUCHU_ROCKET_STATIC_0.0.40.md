# ChuChu Rocket! commercial target — 0.0.40

0.0.40 adds the first productized commercial-disc reconnaissance path through `dc_disc_probe`.

The supplied ChuChu Rocket! CDI is treated only as a local validation input; no commercial game data is redistributed with DreamcastRecomp.

## Disc probe

`dc_disc_probe` currently recognizes CDI/container images that expose a contiguous 2048-byte ISO9660 payload. It locates the Primary Volume Descriptor, reads the root directory, extracts `IP.BIN`, reads the Dreamcast boot filename and locates the boot binary.

For the supplied ChuChu Rocket! image the validation path resolves:

```text
IP.BIN
 -> boot filename: 1ST_READ.BIN
 -> boot binary load address: 0x8C010000
```

The tool also reports SDK marker strings useful for prioritizing hardware work and prints the first SH-4 words from the boot binary.

Use on Windows:

```bat
run_commercial_disc_probe.bat "C:\path\to\ChuChu Rocket!.cdi"
```

or directly:

```bat
build\Release\dc_disc_probe.exe "C:\path\to\game.cdi"
```

## Why ChuChu Rocket!

This image follows the native Dreamcast/Katana route rather than the Windows CE route found in the previously inspected SEGA Swirl edition. It therefore exercises the same PVR/AICA/Maple/Holly/GD-ROM hardware model DreamcastRecomp is already building.

The target progression is deliberately incremental:

```text
recognize disc
 -> load IP.BIN / boot binary
 -> execute native boot bootstrap
 -> first missing Katana/Holly/GD behavior
 -> first asset read
 -> first visible commercial frame
 -> title / Press Start
 -> menu
 -> board + controller
 -> first playable round
```

No title-ID hacks should be introduced. Every behavior exposed by ChuChu must become a reusable runtime feature plus an independent regression.

## Next missing commercial-loader layer

0.0.40 analyzes and extracts the image. It does **not yet emit a native executable directly from raw `1ST_READ.BIN`**. 0.0.41 should add symbol-free code discovery and a raw commercial image emitter, followed by the minimum BIOS/GD-ROM/Holly services encountered during real execution.
