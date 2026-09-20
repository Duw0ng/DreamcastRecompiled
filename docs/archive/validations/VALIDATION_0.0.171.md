# DreamcastRecomp 0.0.171 — validation / experimental commercial boot

## Confirmed fixes

- SH-4 static analysis recognizes local signed 32-bit BRAF jump tables (Crazy Taxi 2 regression at 0x8C07FF00).
- ARM7TDMI STM block transfers store R15 as PC+12; regression test added. This unblocks Crazy Taxi 2's AICA sound-driver checksum/call stub.
- Experimental CT2 generated build includes the validated modem/G2 aperture behavior, commercial staging fix, mutable PC-relative RAM literal reads, and the indirect guest targets discovered during boot.

## Crazy Taxi 2 milestone

The generated experimental build has progressed through commercial bootstrap, GD-ROM loading, AICA/ARM7 initialization, NOW LOADING, continuous TA/PVR rendering, Maple enumeration and VMU selection UI.

## Known provisional item

The CT2 generated build reconstructs the guest Shinobi allocator state if syMallocInit was skipped. This is deliberately labeled diagnostic/provisional; the allocator implementation executed afterwards is still the game's recompiled SH-4 code.
