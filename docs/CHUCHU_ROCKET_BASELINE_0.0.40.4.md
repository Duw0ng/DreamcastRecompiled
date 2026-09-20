# ChuChu Rocket! commercial baseline — 0.0.41

The supplied user-owned ChuChu Rocket! CDI is now part of the **local acceptance target**. No commercial image or extracted game data is distributed with DreamcastRecomp.

## Disc identification

`dc_disc_probe` 0.0.41 recognizes the supplied image as a native Dreamcast/Katana title and reports:

```text
Volume ID:          CHU_CHU_ROCKET_KAL
hardware:           SEGA SEGAKATANA
maker:              SEGA ENTERPRISES
device:             743C GD-ROM1/1
regions:            JUE
product/version:    MK-51049  V1.007
release date:       20000217
boot filename:      1ST_READ.BIN
title:              CHUCHU ROCKET
boot size:          1532256 bytes
boot load address:  0x8C010000
```

The image contains the expected Katana-era SDK markers (`KAMUI`, `Ninja`, `Shinobi`, `gdFs`, peripheral libraries, `sd`, `syStart`, `syG2`, etc.). That keeps ChuChu on the native Dreamcast hardware route rather than the Windows CE route.

Local validation hashes for the supplied copy:

```text
CDI:      6e95281b2feaa98e8b1327b33d8e60147c329cd314f633bba88eb25229031d3b
IP.BIN:   9925c18f0857ccd363cb8d641122f410083b133102ac99f06bae6bb83f9aad4d
BOOT.BIN: b43cb7977871e0c1ce7971da3da7f5a7ea4eb6903a61fbefc5a50de4230d2927
```

## New raw bootstrap probe

0.0.41 adds `dc_raw_boot_probe` plus `run_commercial_bootstrap_probe.bat`.

This is the first commercial path that no longer depends on ELF symbols. Starting from raw `1ST_READ.BIN` at `0x8C010000`, it conservatively follows:

- direct SH-4 branches;
- direct BSR calls;
- PC-relative literal loads;
- indirect JSR/JMP/BRAF/BSRF when the target register can be resolved locally;
- delay-slot control flow.

On the supplied ChuChu boot, the very first bootstrap redirect is resolved automatically:

```text
0x8C01001C -> 0x8C0DA540
```

The larger 0.0.41 discovery run reached the configured safety ceiling:

```text
Reachable blocks:     4096
Unique instructions:  31540
Known SH-4:           31128
Unknown/ambiguous:    412
Direct calls:         256
Indirect resolved:    791
Indirect unresolved:  286
Local targets:        4569
Out-of-image edges:   19
```

The first 80 instructions of the shallow bootstrap path are decoder-clean. The larger raw walk is intentionally conservative and currently has no section/symbol metadata, so the 412 `Unknown` words are **not yet classified as missing SH-4 ISA**: several cluster inside regions that may be data/jump tables incorrectly reached by raw heuristics. 0.0.41 must separate executable blocks from raw data before those words can be treated as true decoder failures.

## What this proves / what it does not

0.0.41 now proves:

```text
CDI
 -> ISO9660
 -> IP.BIN
 -> 1ST_READ.BIN
 -> raw SH-4 entry
 -> real bootstrap redirect
 -> thousands of symbol-free candidate blocks/transfers
```

It does **not** yet generate the final native C++ program from the raw commercial binary. The normal recompiler still expects ELF function metadata. That gap is the explicit 0.0.41 milestone rather than being hidden behind demo-only progress.

## Windows command

```bat
run_commercial_bootstrap_probe.bat "C:\path\to\ChuChu Rocket!.cdi"
```

The script locally extracts `IP.BIN` / `BOOT.BIN` under `generated\commercial_bootstrap\` and then runs symbol-free raw discovery.
