# DreamcastRecomp roadmap — after 0.0.44

## 0.0.44 — checkpoint complete

- Commercial callback-object discovery without title-specific seeds.
- Argument-passed callback recovery.
- VBR code-template discovery and exact runtime relocation aliasing.
- Previous `0x8C0F05A0` failure passed.
- Copied interrupt entry `0x8C00FA00` passed through template `0x8C14094C`.
- Handler `0x8C13E090` reached and executed.
- Zero-RAW commercial closure and full KallistiOS/CTest regression retained.

## 0.0.45 — immediate target

Make PC-relative literal function pointers inside reachable CFG fragments participate in raw closure discovery. The first acceptance target is `0x8C1090FA -> 0x8C1414DE`, but the implementation must be generic and must preserve `RAW_SH4=0`.

Continue execution after that target and classify the next boundary as one of: missing code discovery, SH-4 exception/interrupt semantics, Holly/PVR/Maple, GD-ROM/G1, or AICA.

Also investigate the commercial ARM7 `0x00200000` unmapped access. Keep it separately measurable so an AICA workaround cannot hide an SH-4/PVR regression.

## 0.0.46+

Priorities remain driven by the first real commercial blocker rather than by pre-implementing the full console:

1. Sustain commercial execution through repeated interrupts and dynamic callbacks.
2. Complete the PVR/TA path far enough for a recognizable first commercial frame.
3. Stabilize Maple input in the commercial SDK path.
4. Finish commercial AICA/ARM7 boot and audio.
5. Reach title/menu navigation, then a first playable ChuChu board.

The 155 KallistiOS ELFs remain the permanent broad regression corpus at every checkpoint.
