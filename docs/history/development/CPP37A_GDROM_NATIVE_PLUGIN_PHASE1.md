# cpp37a — GD-ROM Native Plugin, Phase 1

cpp37a starts the second major ABI2 device extraction after Maple. `dcr_gdrom.dll` now owns physical-media description and bulk data I/O while Guest ABI1 stays frozen.

## Plugin-owned state

- DCR_DISC_MAP_V1 / V2 parsing
- image path and persistent media stream
- physical track geometry
- global track numbering normalization
- session table
- boot-track metadata
- TOC construction for low/high density areas
- bulk logical data reads across 2048-byte guest sectors
- GD-ROM trace counters

## Runtime compatibility mirror

`DreamcastRuntime.dll` still keeps its historical GD fields because CDDA lifecycle, Katana command state and some status commands are not migrated in cpp37a. When the plugin loads, Runtime receives a snapshot of track/session geometry. This allows all old CDDA/status code to continue unchanged while physical reads and TOC use the plugin.

If `dcr_gdrom.dll` is missing, cpp37a falls back to the exact built-in parser/read/TOC path.

## ABI

No `DCRuntime` field was added or removed. Guest ABI1 fingerprint remains:

`00ea21b77c60d564cf2c455c9c9d870892047aea3c3a49f28bcc844b6e04054f`

The pinned ChuChu SAFE guest remains byte-identical.

## Trace

Development runner:

```bat
run_game.bat "C:\path\ccr.cdi" --reuse --gdrom-trace
```

Portable launcher: enable **GD-ROM Trace (verbose)**.

Typical output:

```text
[PLUGIN] loaded ABI2 kind=4 name=DreamcastRecompiled GD-ROM build=cpp37a-gdrom-native-phase1-v1
[PLUGIN V2 gdrom] cpp37a native media map active | tracks=18 | sessions=2 | boot-fad=...
[PLUGIN V2 gdrom] TRACE READ fad=... off=0 bytes=16384 sectors=8
[PLUGIN V2 gdrom] TRACE TOC area=1 session=2 first=... last=...
```

## Next phase

cpp37b should move GD command transaction state and multi-transfer / G1 DMA ownership behind `dcr_gdrom.dll`. cpp37c can then migrate CDDA transport and raw-audio sector ownership.
