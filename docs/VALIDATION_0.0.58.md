# DreamcastRecomp 0.0.59 validation

- Core regression suite: 47/47 PASS.
- Generated-runtime compile/link smoke: PASS.
- Synthetic low-level Maple DMA VMU test: PASS for controller A1 advertisement, VMU DEVINFO, GETMINFO, block 255 read, four 128-byte write phases, BSYNC, and readback.
- VMU image size: 131072 bytes.
- PVR/AICA/timing paths unchanged from 0.0.57.
