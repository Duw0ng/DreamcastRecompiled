# SGGG cpp18 validation checkpoint

Base: `DreamcastRecomp_v0.1.1_SGGG_cpp17_MENU_checkpoint`.

## New change

Adds the opt-in runner flag:

`--probe-sggg-newgame`

The flag reuses the already-existing low-level Maple probe state without changing
`DCRuntime` ABI. It emits controller input through the real Maple GETCOND path:

- START: PVR frames 1800-5000, short pulse every 90 GETCOND polls.
- A: PVR frames 2750-20000, short pulse every 48 GETCOND polls.

Normal/manual controller behavior is unchanged when the flag is absent.
The old CT2 and START-only probes keep their original behavior when used alone.

## Closure

- Reachable functions: 6393
- SH-4 instructions: 676914
- RAW_SH4 / unknown instructions: 0
- `0x8C182864` and `0x8C212C58` remain part of the validated native closure.

## Build/tests

- Source build: RC=0.
- Full test inventory: 54 tests.
- Tests 1-52 passed in the main run; tests 53-54 were rerun separately and passed.
- `generated_compile_test`: RC=0.
- Existing cpp17 SH-4 shards were reused; only generated runtime/runner had to be rebuilt for this probe change.

## Runtime validation

Command profile:

- commercial boot + SGGG disc map
- PVR multithreaded
- fast + direct dispatch
- ARM7 enabled
- host device clock
- SH-4 tick batch 256
- `--probe-sggg-newgame`

Result after the controlled 180-second run:

- ~8011 PVR frames / ~7988 flips
- New Game selected successfully
- multiple post-menu dialogue scenes rendered correctly
- no missing SH-4 target observed
- no DreamcastRecomp fatal/runtime error observed

The included screenshot `SGGG_CPP18_NEWGAME_8000.png` is from the end of that
validation run. The full runtime log is included as
`SGGG_CPP18_RUNTIME_180S.log`.

A separate exploratory run using the older combined probes reached ~13314 frames
without a missing target, providing additional confidence that cpp17/cpp18 can
continue through the early New Game tutorial path.
