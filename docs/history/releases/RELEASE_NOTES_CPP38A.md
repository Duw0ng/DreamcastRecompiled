# DreamcastRecompiled v1.2 cpp38a

**PVR_TA_NATIVE_FASTPATH — Phase 1**

- One-copy Store Queue → TA staging.
- Bulk main-RAM CH2 → TA capture.
- Bulk main-RAM CH2 → VRAM texture upload.
- SSE2 Type-7/8 EndOfStrip/source scan.
- Homogeneous-source complete-strip decode shortcut.
- New `pvr38=on/capture1copy/ch2bulk/simdtag/homstrip` heartbeat marker.
- One-time CH2 bulk activation log markers.
- Short application title remains `DreamcastRecompiled - <GAME> [cpp38a]`.
- GD-ROM cpp37c remains unchanged.
- Guest ABI1 and SAFE ChuChu guest unchanged.
