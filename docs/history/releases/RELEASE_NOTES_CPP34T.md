# cpp34t — second Late-AOT target 0x8C036118

Base: cpp34s V42/LAPM reliability + cpp34r network fast path.

The cpp34s online registration submit progressed past the previous V.42/PPP/TCP stall:
- START generated a new LAPM I-frame.
- PPP/IP/TCP counters advanced from 11/6/5 to 12/7/6.
- 52 bytes were delivered from Dreamcast TCP to the host socket.
- The remote server returned 16 bytes and then FIN.

The next blocker was no longer networking. The guest attempted a native call to 0x8C036118 while only 0x8C036102 was present in the Late-AOT overlay.

cpp34t changes only the tiny Late-AOT overlay workflow:
- supports repeated `--target` arguments;
- builds 0x8C036102 and 0x8C036118 into one overlay;
- preserves the existing 5185-function guest cache;
- preserves cpp34s network/runtime behavior and ABI1;
- no full ChuChu recompilation is required.

Run:

```bat
TEST_CPP34T_LATE_AOT2_ONLINE.bat "C:\ruta\ccr.cdi"
```
