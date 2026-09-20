# DreamcastRecomp 0.0.53 validation

## Commercial closure

ChuChu Rocket! / supplied CDI, synthetic bootstrap at `0x8C008000`:

- closure passes: 22;
- reachable functions: 2,245;
- call-graph edges: 4,161;
- reachable instructions: 168,598;
- known SH-4: 168,598;
- unknown SH-4: 0;
- `RAW_SH4=0`;
- dense dispatch targets: 223.

The reported post-START target `0x8C0265DA` is present without a manual game-address seed. `0x8C026610` is also present. `0x8C026F2E` is intentionally excluded from ordinary promotion because direct recursive expansion currently creates 4,430 false RAW operations.

## Generated C++

The final generated commercial source compiled successfully with Clang 17 on Linux. The runtime dispatch-cache code and the generated 2,245-function program both compiled and linked into `dreamcast_program`.

## Performance probe

Headless commercial probe, same CDI, ARM7 enabled, host device clock, catch-up cap 4,096, stop at 900 TA packets:

- 0.0.53: 25.58 s;
- 0.0.52 comparison: did not reach 900 packets inside a 90 s external limit in the same debug-build environment;
- representative 0.0.53 heartbeat at 849 packets: 17,772,567 dispatch-cache hits / 1,222 misses;
- 900-packet endpoint: 23 logical frames, 23 renders, 22 page flips;
- ARM7 endpoint: 3,170,236 executed instructions; host timing remains bounded and reports dropped host-only debt.

This is a development benchmark, not a Windows FPS guarantee. The next user run is the authoritative interactive performance comparison because it includes the Win32 PVR window, keyboard/XInput and presentation path.

## Regression

- CTest: 47/47 PASS.
- Commercial recompile: 0 unknown / `RAW_SH4=0`.
- Generated commercial compile/link: PASS with Clang 17.

## Pending acceptance

Interactive START cannot be considered passed until the Windows run confirms it. The local static closure proves that the exact old missing target is registered, but the next runtime-selected boundary may occur immediately afterwards.
