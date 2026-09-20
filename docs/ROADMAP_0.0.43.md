# DreamcastRecomp roadmap from 0.0.43

## 0.0.43 — checkpoint released

- External CDI/FAD disc map.
- First BIOS GD-ROM request/status/server HLE.
- REQ_MODE and SET_MODE confirmed in the supplied retail run.
- Register-indirect `JSR @Rn` remains runtime-dynamic.
- First SH-4 CH2/Holly interrupt plumbing is present.
- Commercial continuation limit is optional instead of a hard 4096-stage stop.
- 47/47 CTest and 155/155 KallistiOS ISA regression pass.

## 0.0.44 — data-driven commercial closure

Primary target: recover `0x8C0F05A0` and similar runtime function pointers generically.

- Identify Katana object/vtable/callback tables feeding `JSR @Rn`.
- Promote only code-like targets with control-flow evidence; avoid data-as-code closure explosion.
- Re-run ChuChu until the first actual sector-read request or next hardware boundary.
- Add regression fixtures for the new indirect-dispatch pattern.

## 0.0.45 — commercial data loading and DMA validation

- Exercise direct and multi GD-ROM reads against the original CDI.
- Validate G1/GD completion semantics and Holly GD-DMA event handling.
- Validate SH-4 DMAC channel 2 against retail traffic.
- Preserve deterministic diagnostics for FAD/count/destination and DMA completion.

## 0.0.46 — first useful retail PVR output

- Drive TA packet parsing from commercial DMA/store-queue traffic.
- Validate render-start/render-done/VBlank/page-flip interrupts.
- Turn the PVR window into the authoritative commercial video output.
- Goal: first recognizable ChuChu Rocket! frame.

## 0.0.47+

- Commercial Maple/controller path.
- AICA/ARM7 commercial SDK audio path.
- Title/menu navigation.
- Asset loading correctness.
- First board/gameplay state.

The 155 KallistiOS ELF corpus remains a permanent regression suite throughout the commercial roadmap.
