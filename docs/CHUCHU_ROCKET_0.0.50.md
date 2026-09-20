# ChuChu Rocket! commercial status — 0.0.50

## Milestone

0.0.50 moves the retail path from a software-render heuristic to the actual Katana PVR event chain far enough for the guest to start rendering itself.

The Windows 0.0.49 experiment was decisive: without frame-sync the corrected TA parser produced a recognizable white SEGA splash, but a gray 192x224 region revealed that the live framebuffer was being presented mid-registration. With frame-sync enabled, the host instead showed the untouched `#101018` clear buffer because our scene lifecycle had not yet reached the guest render-start path.

## Root cause

The TA parser recognized list termination internally but did not raise the corresponding Holly normal interrupts. Katana waits on those completion events before advancing its render state machine. Consequently `TA_LIST_INIT` and geometry could advance while `isp-start` remained zero.

## 0.0.50 behavior

- End Of List is recognized from the TA parameter type, not by requiring an all-zero PCW.
- Opaque, opaque-modifier, translucent, translucent-modifier and punch-through lists raise their real Holly event bits.
- Katana receives those events and writes `STARTRENDER` itself.
- Synchronous host rendering raises Video/ISP/TSP render-done bits (normal bits 0-2); the retail handler has been observed acknowledging `0x7`.
- `TA_LIST_INIT` is treated as the strong registration boundary once seen, so list-number wrap no longer tears an active commercial scene.
- `TA_LIST_INIT` and `STARTRENDER` command writes self-clear in the MMIO shadow.
- PVR live-window message pumping is independent of frame presentation.

## Acceptance snapshot

At an intentional 1,200-TA-packet probe limit:

- closure: 2,194 functions / 165,555 known SH-4 / 0 unknown / RAW=0;
- TA init: 38;
- list-end events: 93;
- guest STARTRENDER: 31;
- total render starts: 32 (31 guest + 1 initial heuristic bridge);
- render completions: 32;
- page flips: 31;
- framebuffer read-base writes: 68;
- geometry: 853 vertices / 410 triangles / 15 sprites.

The completed framebuffer contains the full SEGA splash with white background, blue logo and gray `PRESENTED BY`; the old magenta fallback and right-side partial gray block are absent in the completed frame.

## Long-run 0.0.49 feedback folded into 0.0.50

A later Windows run reached 5,417 TA packets / 158 frames but exposed two independent problems: the next BIOS GD-ROM command was `0x15`, and the PVR window updated in multi-second bursts while the interpreted ARM7 accumulated more than five billion instructions. 0.0.50 therefore also:

- models the CDDA command-state path through `PLAY_TRACKS`, `PLAY_SECTORS`, `PAUSE`, `RELEASE` and `STOP`;
- uses the 22.5792 MHz AICA/ARM7 clock and 512 clocks per 44.1 kHz sample;
- bounds host-clock ARM7 catch-up and reports discarded host-only debt as `host-aica-drop`;
- pumps Win32 PVR messages during ARM7 scheduling as well as normal guest runtime ticks;
- never sleeps the emulation thread merely to impose the PVR-window 60 Hz cap.

The CDDA model currently preserves drive/playback state; it does not yet decode/output raw CDDA tracks from the CDI.

## Known limits

This is not yet a claim of full PVR or audio timing accuracy. One initial heuristic render remains, later post-splash screens still need Windows validation, framebuffer scanout is simplified, and the bounded host AICA catch-up intentionally trades wall-clock audio fidelity for responsiveness until ARM7 has a faster execution backend.
