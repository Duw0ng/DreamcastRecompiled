# DreamcastRecomp 0.0.41 — validation

## Windows SFX feedback from 0.0.40.4

The dedicated WinMM worker architecture is validated by the user's Windows run:

- producer gap max: 1068 ms
- WinMM worker gap max: 46 ms
- underrun restarts: 1
- ring reached its 16-chunk cap
- 2557 starvation events / 5913 silence chunks / 4280 freshness overruns
- audible SFX remained functional and subjectively much better than 0.0.40.3

This confirms that the worker decouples the Windows device from >1 second producer stalls. The remaining problem is producer pacing/burstiness, not direct WinMM delivery. We intentionally do not increase buffering again because that would reintroduce interactive latency.

## KallistiOS regression

The existing Linux regression suite remains 47/47 PASS. The supplied 155-ELF KallistiOS corpus remains the permanent compatibility corpus.

## ChuChu Rocket! raw commercial path

The supplied CDI is used only locally for validation and is not redistributed.

The raw commercial recompiler now turns the extracted 1ST_READ.BIN into a synthetic symbol/function map, DCIR and generated C++ without requiring an ELF symbol table. Local validation reached approximately:

- 695 synthetic entry candidates
- 727 reachable functions
- 1,182 call-graph edges
- 50,752 reachable SH-4 instructions
- 50,752 known / 0 unknown in the selected reachable closure
- 9,036 CFG blocks
- 50,735 DCIR ops
- RAW_SH4 = 0

The generated commercial C++ project compiles successfully on Linux. Runtime execution now proceeds into the Katana bootstrap. The current blocker is no longer instruction decoding; it is reconstructing the Dreamcast/Katana boot environment expected before 1ST_READ.BIN begins (system RAM globals, syscall/boot state and then hardware services).

A P1/P2 code-alias canonicalization path was also added because Katana startup intentionally executes cache-sensitive helpers through the uncached 0xA... alias while the raw image is mapped at 0x8C....

## Acceptance for 0.0.41

0.0.41 is considered successful when:

1. Homebrew regressions stay green.
2. Raw commercial input can be extracted from CDI locally.
3. Symbol-free commercial code can be converted to generated C++ with no RAW_SH4 in the selected reachable closure.
4. The generated commercial project itself compiles.
5. Runtime reaches a concrete boot-environment/hardware dependency rather than failing at file/container recognition.
