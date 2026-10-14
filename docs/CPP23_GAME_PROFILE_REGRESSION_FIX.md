# cpp23 — per-game profiles + commercial boot regression repair

Base: `DreamcastRecomp_v0.1.1_cpp22_GPU_TEXTURE_CACHE_HOTFIX_checkpoint`.

## Regression audit

The first broad compatibility regression after the GitHub/v0.1.1 baseline is introduced by **cpp11**.

In v0.1.1, `dc_setup_commercial_boot()` only performed Dreamcast 32-byte scramble restaging when `runtime.ct2_compat` was enabled. Daytona and Lodoss used `--commercial-boot` without CT2 compatibility, so their historical path stayed plain.

cpp11 changed that behavior from CT2-only to generic:

- new `commercial_boot_image_address` / `commercial_boot_image_size` metadata;
- every generated commercial image with a non-zero `.raw_boot` was restaged as scrambled;
- the first cpp11 implementation also included the synthetic 32 KiB IP.BIN region in that restaging.

That was correct for the SGGG path being developed at the time, but it changed previously working titles globally.

Later repairs were partial:

- **cpp15** stopped scrambling the first 32 KiB IP.BIN and kept only the main executable in the generic restaging range.
- **cpp21** added `--commercial-boot-stage={auto,plain,scrambled}` and used disc geometry in `auto`.
- cpp21 restored cooked 2048/0 ChuChu behavior, but raw 2336-style titles still default to scrambled even when their pre-cpp11 launcher path was historically plain.

This explains the current symptom: ChuChu can be healthy again while other games that had reached a useful milestone before cpp11 can regress.

## cpp23 fix

Boot staging is no longer treated as one global compatibility rule by the launcher. It is now part of a persistent per-game profile.

Known defaults:

| Game/profile | renderer | boot stage | important compatibility state |
|---|---|---|---|
| ChuChu Rocket | dx11 | plain | cooked retail path |
| Crazy Taxi 2 | dx11 | scrambled | CT2 compatibility + CT2 seeds |
| Daytona USA / PIZZICATO POLKA | software | plain | restores pre-cpp11 commercial-boot behavior |
| Record of Lodoss War | software | plain | direct secondary executable path |
| Segagaga / SGGG | software | scrambled | preserves cpp17/cpp18 validated restaging |
| Unknown title | dx11 | auto | conservative generic profile, editable |

The runtime cpp21 staging selector is retained. cpp23 changes **which mode the launcher requests per title**.

## Persistent per-game workspace

The first run of a title creates:

```text
games/
  <DISC_TITLE>/
    game.cfg
    generated/
      IP.BIN
      BOOT.DISC.BIN
      disc.map
      BOOT.BIN
      BOOTSTRAP.BIN
      function_map.csv
      cpp/
    build/
    logs/
    captures/
```

`game.cfg` is generated from the title/volume discovered by `dc_disc_probe`. Existing values are preserved on later runs.

`--reuse` and `--clean` now operate only on the detected game's workspace. This prevents one title's generated closure/build from being reused accidentally for another title.

## Editable `game.cfg`

Important keys:

```ini
mode=GENERIC
renderer=dx11
commercial_boot=1
commercial_boot_stage=auto
direct_game_entry=
ct2_compat=0
seed_file=
entry_seed=0x8C010000
raw_base=0x8C008000
raw_entry=0x8C008300
max_functions=8192
max_closure_entries=6144
closure_passes=64
audio_profile=full
device_clock=host
special_boot_file=
controller_profile=
extra_run_args=
```

A new/unknown game therefore gets a safe reproducible profile instead of changing hard-coded launcher branches.

## Validation in this environment

- CMake configure: PASS.
- `dc_disc_probe`: builds.
- `dc_boot_prepare`: builds.
- `dc_raw_recomp`: builds.
- `cpp_emitter_tests`: PASS.

Real Windows/D3D11/gameplay acceptance still has to be performed on the user's Windows machine with the owned CDI images.
