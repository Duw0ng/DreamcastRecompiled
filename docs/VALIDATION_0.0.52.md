# DreamcastRecomp 0.0.52 validation

## Scope

0.0.52 validates two changes exposed by the Windows 0.0.51 acceptance run: the next symbol-free nested dispatch target and the PowerVR background plane used by the Sonic Team splash.

## Commercial closure

The final raw commercial closure is expected to report:

- 2,242 reachable functions;
- 168,547 known SH-4 instructions;
- 0 unknown SH-4 instructions;
- `RAW_SH4=0`.

`0x8C0264D2` is discovered through the nested row/method table and is not a manual seed.

## PVR visual acceptance

The runtime now resolves the hardware background strip from `PARAM_BASE + ISP_BACKGND_T` and parameter memory. A local Sonic Team capture uses a cyan background around RGB `118,200,253` instead of the old host clear `#101018`.

A bounded 9,000-packet run reached 483 frames without the 0.0.51 target failure. A longer run exceeded 13,600 packets / 586 frames and reached the ChuChu Rocket! title with `PRESS START BUTTON!`.

## Known limitations

Some title-screen textured elements still contain black rectangular regions. This is intentionally left visible for follow-up texture/alpha/addressing work rather than hidden by a title-specific workaround.

Commercial AICA output is not yet claimed correct even though ARM7 execution continues.

## Regression results

Final 0.0.52 source validation:

- CTest: 47/47 PASS;
- KallistiOS cache: 155/155 ELF loaded and ISA-clean;
- KallistiOS known SH-4 instructions: 17,953,662;
- KallistiOS unknown SH-4 instructions: 0;
- 155/155 `_main` call graphs with `RAW_SH4=0`;
- standard KOS `sound/sfx`: runner RC=0, 182 reachable functions, `RAW_SH4=0`, ARM7 faults=0, native-starts=1, 3,366 mixed frames and 2,602 nonzero PCM frames;
- final generated commercial compile test: RC=0.
