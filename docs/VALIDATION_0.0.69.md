# Validation — 0.0.69

## Scope

0.0.69 generalizes the commercial-disc path beyond contiguous 2048-byte payloads.
`dc_disc_probe` now parses DiscJuggler CDI v2/v3/v3.5-4 track metadata and emits
`DCR_DISC_MAP_V2`, while the generated GD-ROM HLE maps 2048-byte guest sectors
through each track's physical sector size and user-data offset.

Supported data geometries in this milestone:

- 2048-byte sectors, user offset 0;
- Mode 2 / 2336-byte sectors, user offset 8;
- Mode 1 / 2352-byte sectors, user offset 16;
- Mode 2 / 2352-byte sectors, user offset 24.

ISO9660 probing supports both normal relative extents and multisession absolute
disc-LBA extents. This matters for self-boot CDI layouts where, for example, a
track can begin at LBA 11700 while the ISO root extent is 11723.

## Known retail reference

The existing ChuChu Rocket CDI is parsed through the CDI table rather than the
old signature-only scanner:

- 18 tracks found;
- boot track S1/T1;
- physical sector 2048;
- data position `0x4B000`;
- FAD 150;
- volume `CHU_CHU_ROCKET_KAL`.

The newly extracted IP.BIN and 1ST_READ.BIN are byte-identical to the 0.0.68
commercial inputs (matching SHA-256), preserving the established commercial
closure.

## Mode2/2336 validation

A synthetic DiscJuggler v3.5/4 CDI was built with:

- Mode 2 / 2336 physical sectors;
- 8-byte user-data offset;
- track start LBA 11700 / FAD 11850;
- IP.BIN in physical sector 0;
- PVD in physical sector 16;
- ISO root/boot extents encoded as absolute disc LBAs.

`dc_disc_probe` found the boot track, parsed the absolute extents, extracted the
boot executable and emitted a V2 map.

Separately, a generated runtime loaded a V2 map with boot FAD 45150 and performed
a GD-ROM PIO read from a synthetic Mode2/2336 image. The bytes delivered to
Dreamcast RAM matched the 2048-byte user payload exactly after skipping the
8-byte physical-sector prefix.

## Regression

`ctest --output-on-failure`: **47/47 PASS**.

A fresh generated sample runtime was compiled and linked after the V2 GD-ROM
changes (`dreamcast_generated`, `generated_compile_test`, and
`dreamcast_program`).
