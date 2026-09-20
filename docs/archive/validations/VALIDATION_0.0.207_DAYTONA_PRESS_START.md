# DreamcastRecomp 0.0.207 - Daytona USA PRESS START / Attract Validation

Date: 2026-09-18

## Result
The uploaded CDI was validated through the real Daytona USA startup flow:

CRI/ADX -> Genki/WAVE MASTER -> Memory Card -> no-save confirmation -> NOW LOADING ->
Daytona USA attract/gameplay scene -> PRESS START BUTTON.

The attract demo continued rendering moving 3D race scenes after PRESS START without an
immediate SH-4, GD-ROM, Maple or PVR failure.

## Identity
Although the selfboot IP.BIN metadata reports `PIZZICATO POLKA`, the commercial payload
contains explicit `DAYTONA USA`, `DAYTONA USA REPLAY` and `DAYTONA USA GHOST` strings,
and the rendered title/attract screen is Daytona USA.

## Closure
Final validation closure after adding the late target `0x8C044740`:

- Registered/reachable functions: 3270
- Reachable SH-4 instructions: 258737
- Known SH-4: 258737
- Unknown SH-4: 0
- RAW_SH4: 0
- Manual seed entries: 21
- Closure passes: 11

`0x8C044740` also exposed `0x8C047198` and 52 additional target registrations in the
incremental validation build.

## Runtime validation
The earlier attract build reached 59 GD-ROM requests / about 15.6 MB read before hitting
late callback `0x8C044740`. After adding that callback, the rerun returned to PRESS START,
continued animating the attract demo, and passed that former failure point with no
`DreamcastRecomp ERROR` or `SH4-FAULT` during the stability window.

Two framebuffer samples taken 12 seconds apart had different SHA-256 hashes, confirming
that the attract scene was actively rendering rather than showing a frozen frame.

## Windows reproduction
Use:

    run_daytona_0.0.207.bat "C:\path\Daytona USA.cdi"

The batch extracts the user's disc, prepares the bootstrap, supplies the full validated
seed set, compiles the generated C++ on Windows and launches the commercial runner with
`--direct-game-entry=0x8C010000` and deterministic `--device-clock`.

The game image itself is NOT included in this package.
