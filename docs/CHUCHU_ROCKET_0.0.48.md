# ChuChu Rocket! commercial checkpoint — DreamcastRecomp 0.0.48

## What changed

0.0.48 turns the 0.0.47 "VBlank alive, TA idle" checkpoint into sustained commercial PVR activity.

The first blocker was G2 DMA channel 0. Katana wrote the SPU/AICA DMA `START` flag and waited for hardware to clear it. DreamcastRecomp previously treated the register as persistent MMIO. The runtime now performs the transfer, clears `START`, and raises the matching Holly event.

A later failure initially looked like low-RAM/VBR corruption. The actual cause was copied exception code: Katana relocates an interrupt wrapper from its original image into VBR-relative RAM. The wrapper uses `MOVA` and a PC-relative literal slot to preserve interrupted `R0`. Generated C++ had baked the original template address/value, so an IRQ could change a decompressor counter from a small value to zero, after which `DT` wrapped it to `0xFFFFFFFF` and a runaway copy eventually overwrote system RAM. 0.0.48 makes those PC-relative operations relocation-aware instead of protecting RAM or re-injecting vectors.

## Verified progression

```text
G2/SPU DMA completes
  -> AICA firmware/data upload progresses
  -> relocated IRQ wrapper preserves interrupted registers
  -> GD-ROM startup/data loads continue
  -> Store Queue + PREF traffic reaches TA
  -> repeated logical PVR frames/renders/page flips
```

Final local closure:

```text
2194 functions
165555 known SH-4 instructions
0 unknown
RAW_SH4=0
```

Bounded native high-water mark:

```text
429 TA packets
9 logical PVR frames
9 renders
9 page flips
```

This checkpoint deliberately does not add ChuChu-specific runtime addresses. Newly discovered functions come from structural callback/dispatch patterns in the raw image.

## Next boundary

The next useful work is no longer "make TA receive anything." It is to validate the actual commercial render-start/scene semantics and first visible frame, then continue the ARM7 instruction gap and controller/game-loop path toward the first playable board.
