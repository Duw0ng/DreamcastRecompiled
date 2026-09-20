# VALIDATION 0.0.117

- 47/47 CTest PASS.
- ChuChu Rocket commercial closure: 2965 registered functions, 295215 known SH-4 instructions, RAW_SH4=0, 32 shards.
- `dc_runtime.cpp` from the commercial closure compiles in Release; commercial shard compilation proceeds without errors. Full optimized closure build can exceed the execution window of the validation host.
- Deferred PVR state is indexed per surface/texture epoch; GPU and MT fallback both resolve the same indexed state/snapshot.
- TA/PREF experiments from 0.0.113 remain absent.
- Windows build remains serial/low-memory.
