# Validation — cpp38b1 PVR_SUPERBLOCK_RECOVERY

## Source/config checks

`VERIFY_CPP38B1.py` passes every check:

- cpp38b mapped-index builder absent from Runtime and generated-runtime emitter.
- cpp38a `index_scratch` construction + one bulk D3D11 index upload restored.
- FPU `superblock` default restored in Runtime and generated-runtime emitter.
- FPU metrics default OFF.
- host/AICA sync default 262144 cycles with environment A/B override retained.
- run_game and portable launcher select cpp38b1 / superblock / metrics-off / 262144.
- Guest ABI v1 fingerprint exact MATCH.
- pinned SAFE ChuChu guest exact SHA-256 MATCH.

## ABI / guest preservation

Guest ABI v1 fingerprint:

```text
00ea21b77c60d564cf2c455c9c9d870892047aea3c3a49f28bcc844b6e04054f
```

Pinned ChuChu `dcr_game_module.dll` SHA-256:

```text
ed43078f57f3120d8510d40f024f822bbd0b6059be03ef369f8d82761ae5c1f4
```

No SH-4 guest rebuild is required for the normal `--reuse` test.

## Build validation

- Top-level Release build: PASS.
- Main CTest run reached 53/54 before the environment command time limit; `generated_single_build` had already PASSed.
- `generated_program_build` was then run separately and PASSed in 20.12 s.
- Effective suite: **54/54 PASS**.
- Runtime Release build: `DreamcastRuntime`, `dcr_maple`, `dcr_gdrom`, `dcr_network` PASS on Linux/GCC.
- GCC still emits the pre-existing PPP/vector optimization warning in `modem_ppp_send_payload`; no new cpp38b1 error was introduced.
- Windows portable launcher cross-compiled as PE32+ x86-64 GUI.

## Required Windows live check

The exact D3D11 performance result cannot be established in the Linux validation environment. Run:

```bat
TEST_CPP38B1_FAST.bat "C:\path\ccr.cdi"
```

Expected heartbeat:

```text
build=v1.2-cpp38b1-pvr-superblock-recovery
fpu-mode=superblock
fpu-metrics=off
host-sync-q=262144
pvr38=on/cpp38b1/indexscratch/capture1copy/ch2bulk/simdtag/homstrip
```

Compare the same menu / pre-Mouse-Mania / Mouse Mania scenes against cpp38a and the historical 0.0.156/0.0.165 logs.
