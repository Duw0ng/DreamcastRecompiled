# DreamcastRecomp 0.0.210 — Lodoss low-level START fix

Experimental opt-in fix for **Record of Lodoss War**.

## What changed

`--probe-controller-start-burst` now also reaches the raw Maple `GETCOND` response path. This is required by retail/Katana titles that do not call the high-level KOS `maple_dev_status` function detected by the emitter.

The finite smoke sequence is the proven Lodoss WIP sequence:

- START at GETCOND #8..10
- START at GETCOND #20..22
- START at GETCOND #40..42
- START at GETCOND #80..82
- START at GETCOND #140..142
- START at GETCOND #220..222

Normal runs are unchanged unless the flag is explicitly enabled.

## Easiest test

Run:

`run_lodoss_start_fix_0.0.210.bat "C:\ruta\Record of Lodoss War.cdi"`

The console should print six lines beginning with:

`[DreamcastRecomp smoke] low-level Maple START pulse`

## Linux validation in this package revision

The patched 0.0.210 Lodoss runner compiled and received all six low-level START pulses. The smoke then advanced to a new missing SH-4 target `0x8C02B37C`. That target is **not** included in this START-only fix; it is the next closure issue exposed after the controller injection became functional.
