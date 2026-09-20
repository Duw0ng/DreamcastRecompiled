# ChuChu Rocket! — 0.0.53 checkpoint

## Boundary reported from 0.0.52

The Windows 0.0.52 acceptance reached the ChuChu Rocket! title/menu. Pressing START then produced:

`0x8C0190A4 -> 0x8C0265DA`

The target is the first method in the next SDK state row selected by the two-level dispatcher at `0x8C01909C`.

## Static closure fix

The table analyzer now permits one independently strong adjacent row after a short-wrapper confidence boundary. That exposes the row containing:

- `0x8C0265DA` — clean callable method and the exact reported START target;
- `0x8C026F2E` — compact indexed tail dispatcher;
- `0x8C026610` — clean callable method.

Raw commercial promotion intentionally keeps `0x8C026F2E` out for now. Seeding/expanding that dispatcher directly causes a false closure expansion to 2,302 functions with 4,430 `RAW_SH4` operations. The clean methods are accepted without that regression.

Final 0.0.53 closure for this CDI:

- 2,245 reachable functions;
- 168,598 known SH-4 instructions;
- 0 unknown SH-4 instructions;
- `RAW_SH4=0`;
- `0x8C0265DA` present;
- `0x8C026610` present;
- `0x8C026F2E` intentionally deferred.

## Performance work

The 0.0.52 Windows log contained more than 132 million generated CALLs. Each call previously performed repeated `unordered_map` target/alias lookups. 0.0.53 adds a small direct-mapped dispatch cache while preserving native overrides and relocated-code aliases.

A local headless probe on the supplied CDI reached 900 TA packets in 25.6 seconds with the normal 4,096-step ARM7 host catch-up cap. In the same Clang debug environment, the 0.0.52 comparison did not reach 900 packets within 90 seconds. Near 849 packets, 0.0.53 reported 17,772,567 cache hits and 1,222 misses.

## Windows acceptance

Run `run_commercial_recompiled.bat`, reach the title, press Enter/START, and capture the next failure (if any). The important confirmation is that the old `0x8C0265DA` registration error is gone. The launcher also enables PVR profiling and heartbeat dispatch-cache counters for the speed investigation.
