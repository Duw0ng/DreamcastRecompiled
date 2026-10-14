# cpp37b — GD-ROM Command / Multi / DMA Native

cpp37b is phase 2 of GD-ROM extraction. It preserves the frozen Guest ABI1 and the cpp37a media API while adding `dcr.gdrom.control.v2` for host/runtime-only state.

## Moved into dcr_gdrom.dll

- GD command request ID / busy-complete-continue state
- command params/results mirror
- current FAD and multi-transfer cursor/progress
- PIO/DMA multi-transfer execution
- direct bulk writes to guest RAM through Host Services V2 memory regions
- GD-DMA IRQ raise and G1-DMA completion acknowledgement
- callback/callback-argument state
- runtime-readable plugin counters

## Deliberately still in DreamcastRuntime

- CDDA play/pause/release/stop lifecycle
- CDDA sector decoding, Yellow-Book repair and PCM mixing
- drive status/subcode behavior that depends on the live CDDA state

Those move in cpp37c so cpp37b can be validated without destabilizing working audio.

## Window naming

The cached generated runner may contain an old historical title string. cpp37b overrides it at the Runtime boundary using environment metadata, so no guest relink is needed.

Normal game window example:

`DreamcastRecompiled - CHUCHU ROCKET [cpp37b]`

Portable launcher before probing:

`DreamcastRecompiled Portable [cpp37b]`

After the portable probes the selected disc, it changes to:

`DreamcastRecompiled Portable - <disc title> [cpp37b]`

## Diagnostics

Normal heartbeat now includes entries such as:

`gd-native=phase2`

`gd-plugin=<bulk reads>/<sectors>/<bytes>/tocN/cmdN/syncN`

`gd-xfer=<starts>/<transfers>/dmaN/pioN/irqN/ackN/delegateN`

With `--gdrom-trace`, phase-2 activity additionally prints `CMD request`, `MULTI begin`, `DMA transfer`, `PIO transfer` and `DMA IRQ ack` lines.
