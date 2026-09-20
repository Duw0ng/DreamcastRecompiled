# SFX Windows live progress — 0.0.40.2

The supplied KallistiOS `sound/sfx/sfx.elf` exposed a live-only performance trap.

With `--device-clock-host`, the guest could spend a long time inside KOS BIOS-font rasterization before reaching its controller loop. DreamcastRecomp currently supplies a zero-filled synthetic BIOS font, so that expensive pixel loop produced no useful glyphs. In a host-synchronized run the AICA clock also kept catching up in real time while the SH-4 was doing this redundant work, making the application appear frozen around `_bfont_draw_one_row` (observed guest PC `0x8C0105F0`).

0.0.40.2 registers generic native fast paths for `_bfont_draw_str` and `_bfont_draw_str_ex` while the synthetic font backing is in use. The functions remain semantically display-only for this bootstrap state; execution can now reach Maple input promptly instead of spending host time drawing invisible glyphs.

A second live-only issue was in the host-synchronized device clock. The old catch-up loop advanced its wall-clock anchor by the nominal emulated duration, so wall time spent executing a slow ARM7 catch-up became additional debt on the next sync. On a host that cannot interpret the ARM7 at the nominal AICA rate, this creates a positive-feedback catch-up spiral and can starve SH-4/Maple indefinitely. After each due catch-up slice, 0.0.40.2 now rebases the host anchor and target to the actually completed AICA step count. The clock stays synchronized without recursively charging emulator execution time as new guest time.

This is not an SFX title hack. Multiple KallistiOS demos use the same BIOS-font software renderer, so the same acceleration applies to the corpus until a real/legal font substitute is modeled.

The Windows SFX runner keeps the separate host status window and heartbeat introduced in 0.0.40.1. Heartbeats now include `bfont-fast=<count>`.

Validation on the exact supplied `sound/sfx/sfx.elf`: deterministic mode reaches native AICA PCM; host-synchronized automatic A->Start exits cleanly with `native-starts=1`; and a host-input idle run reaches the controller loop with Maple poll counters increasing continuously. Windows key/audio presentation still requires validation on a Windows host.
