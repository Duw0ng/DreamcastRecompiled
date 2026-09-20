# DreamcastRecomp v0.1 Official — consolidated release

This package consolidates the latest controller/tester hotfix codebase into the single public **v0.1 Official** identity (`0.1.0`). No v0.1.3 functionality was removed; the point-release labels were development packaging labels and are folded into v0.1 Official.

## v0.1 controller tester layout hotfix

- Fixes a WinForms docking overlap visible on some Windows DPI/layout combinations where the tab headers and the learning objective were rendered underneath the 76 px status header.
- The tester now uses a two-row `TableLayoutPanel`: fixed header in row 0 and all tabs in row 1.
- Learning now visibly shows the requested `OBJETIVO - Paso N/...` line before waiting for input.

> **v0.1 learning-start hotfix:** `Iniciar aprendizaje` now updates the objective immediately before waiting for input, keeps all learning state in script scope for Windows PowerShell 5.1 event handlers, and fixes button-31 scanning by using a 64-bit shift mask. The same mask fix is applied to `DreamcastControllerConfig.ps1`. The start button visibly changes to `Aprendizaje activo...` while learning is running.

> **v0.1 learning-UI hotfix:** the Controller Visual Tester now shows live RAW input feedback while guided learning is active. The current step is displayed prominently, wrong/non-matching controls are still reported instead of being silently ignored, and trigger capture shows the candidate axis/delta before release.

# DreamcastRecomp v0.1 Official — native PS4 trigger mapping fix

- Fixes native DualShock 4 L2/R2 cross-talk through DirectInput/WinMM.
- `logical:ltrig` / `logical:rtrig` no longer assume WinMM U/V are independent trigger axes.
- Safe uncalibrated PS4 fallback uses the independent L2/R2 button bits, so pressing one trigger cannot create pressure on the other.
- The visual tester learning mode now includes L2 and R2 analog calibration. It measures the real neutral/full range and saves `range:<axis>:<rest>:<full>` bindings.
- Calibrated ranges support both separate trigger axes and one combined centered DirectInput axis.
- The anti-flicker visual tester changes from v0.1.2 are retained.

## v0.1.2 controller tester anti-flicker hotfix

- Enables WinForms double buffering for the visual controller panel and form.
- Stops invalidating the visual panel on every 33 ms poll; repaint now occurs only when the mapped visual state changes.
- Quantizes analog fingerprinting to visual resolution so harmless HID jitter does not trigger redundant redraws.
- Updates the raw diagnostics text only while its tab is visible and only when content changed.
- Reuses GDI brushes, pens and fonts across paint events and disposes them on close.

## v0.1.2 controller tester Windows PowerShell 5.1 encoding hotfix

- Rewrites `tools/controller_test/DreamcastControllerTest.ps1` as pure US-ASCII source so Windows PowerShell 5.1 cannot reinterpret UTF-8 arrow bytes as typographic quote characters.
- D-pad arrow glyphs are now generated at runtime with `[char]0x2191`, `[char]0x2193`, `[char]0x2190`, and `[char]0x2192`.
- Replaces the long interpolated raw-diagnostics string with a line-array + `-f` formatting path to reduce parser fragility.
- Keeps the official PowerShell parser preflight in `run_controller_test.bat`.


## v0.1.2 controller tester launcher path hotfix

- Fixes the PowerShell preflight in `run_controller_test.bat`: the tester path is now read from the `TESTER_PS1` environment variable instead of `$args[0]`.
- Adds an explicit `if not exist` check before invoking the PowerShell parser.
- Uses `Test-Path -LiteralPath` before `Parser::ParseFile`, avoiding failures with full Windows paths containing spaces or legacy `powershell.exe -Command` argument handling.


## v0.1.2 controller tester parser hotfix

- Fixed the PowerShell parser error in `tools/controller_test/DreamcastControllerTest.ps1` caused by a missing closing parenthesis in the learning-step status string.
- Replaced the fragile nested interpolation with `-f` formatting.
- `run_controller_test.bat` now validates the `.ps1` with the official PowerShell parser before launching it.

# DreamcastRecomp v0.1.2 Official — visual controller diagnostics

- Adds `run_controller_test.bat`, a live Windows controller tester for XInput and native DirectInput/WinMM pads.
- Visualizes Dreamcast-mapped D-pad/buttons, both analog sticks and L/R trigger values.
- Shows raw physical axes/button mask/POV beside the mapped guest values.
- Reads the same `profiles/controller_profile.ini`, backend selection, deadzone and binding syntax used by normal game runs.
- Adds guided input learning for unusual controllers; learned values are only written after explicit Save.
- Intended especially for validating DualShock 4 USB/Bluetooth without DS4Windows.

# DreamcastRecomp v0.1 Official

## Controller compatibility patch

- Adds native **DualShock 4 / PS4** support through Windows DirectInput/WinMM; DS4Windows is no longer required for the normal controller path.
- Fixes the native PS4 D-pad by reading the controller POV hat instead of reusing XInput D-pad button indices.
- Introduces backend-neutral `logical:*` controller bindings used by the default `auto` profile.
- Auto input now tries XInput first and falls back to DirectInput/WinMM, scanning up to 16 legacy joystick slots if the preferred slot is unavailable.
- Adds native DS4 face-button, Options, shoulder, stick and trigger mappings while preserving keyboard and XInput behavior.
- `Configurar_Mando.bat` gains Auto, PS4 native and XInput presets plus native WinMM device scanning.
- Adds controller-codegen regression checks; complete suite remains **53/53 PASS**.

---

# DreamcastRecomp v0.1 Official (0.1.0)

- Promotes the 0.0.210 development lineage to the first official v0.1 release.
- Adds `run_game.bat` as the single supported CDI launcher: `run_game.bat "game.cdi" [--perf] [--debug] [--profile] [--software] [--no-audio] [--clean]`.
- Automatically selects maintained profiles for Daytona USA, Crazy Taxi 2, ChuChu Rocket!, and Record of Lodoss War; unknown discs use the generic commercial path.
- Converts the main historical title launchers into compatibility aliases that forward to `run_game.bat`.
- Adds `check_system.bat`, `VERSION`, release notes and dedicated script documentation.
- Normalizes active build/runtime identity to `0.1.0` / `v0.1 Official`, including generated runner titles, heartbeat build field and session-log filename.
- Replaces the front-page README with an end-user-oriented architecture, usage, compatibility and native-vs-hybrid status guide. The previous milestone-style README is retained at `docs/README_LEGACY_DEVELOPMENT_HISTORY.md`.
- Cleans the public ZIP layout: screenshots move to `test-img/`, 0.0.x validation/history moves under `docs/archive/`, historical patches/seeds are grouped, and internal BAT launchers move under `tools/bat/` while the root keeps only the supported user-facing entry points.

---

## 0.0.207 - Daytona PRESS START / attract gameplay (2026-09-18)
- Daytona USA commercial test now reaches the real `PRESS START BUTTON` attract scene.
- Attract-mode 3D race rendering continues across changing camera/gameplay frames.
- Add a 21-seed commercial closure profile covering the late dynamic callback chain discovered during bring-up.
- Final late target `0x8C044740` also discovers `0x8C047198`; validated closure reaches 3270 functions with `Unknown SH-4 = 0` and `RAW_SH4 = 0`.
- Add `run_daytona_0.0.207.bat` for reproducible Windows compilation/testing from the user's own CDI.
- Add optional `run_daytona_0.0.207_gpu_audio.bat` for GPU/audio experimentation after the validated build.

## 0.0.206 - First Frame (2026-09-18)
- Reach the first visible rendered frame on the uploaded commercial CDI test image.
- Validate deterministic `--device-clock` execution as the key to escaping the VBlank wait loop for this title.
- Absorb additional late SH-4 callback targets discovered during first-frame bring-up.
- Add `run_commercial_recompiled_firstframe_probe.bat` for deterministic first-frame probing.
- Add `VALIDATION_0.0.206_FIRST_FRAME.md` and bundle the resulting first-frame PNG evidence.

# 0.0.205 - 2026-09-18

- Model unattached Dreamcast modem aperture (`0x00600000-0x006007FF`) as zero-read / ignored-write hardware instead of unmapped memory.
- Model G2 External Device aperture (`0x01000000-0x01FFFFFF`) with the same unattached-device baseline.
- Add generated runner option `--direct-game-entry=ADDR` for commercial bootstrap bypass while preserving commercial BIOS/GD-ROM setup.
- Add `run_commercial_recompiled_direct_game.bat`.
- Validated commercial closure at 3177 functions, 245233 known SH-4, 0 unknown, RAW_SH4=0.
- Runtime validation passed former modem/G2 faults and reached GD-ROM reads, code relocation and CH2 texture/TA transfers without a runtime fault during the bounded test.

## 0.0.204 Windows build-path hotfix (2026-09-17)

- Move generated commercial CMake build output from `generated\\commercial_recompiled\\cpp\\build` to root-level `_cb`.
- Fix Visual Studio/MSBuild `FileTracker : error FTK1011` caused by overlong CMake scratch / `.tlog` paths.
- Update all commercial runner/probe BAT files to launch `_cb\\Release\\dreamcast_program.exe`.
- `DCR_CLEAN_RECOMPILE=1` now also removes `_cb`.

## 0.0.204 SH-4 hotfix (2026-09-17)

- Fix runtime-selected short SDK callback rows that could omit a compact `MOV.L literal -> JMP @Rn` veneer from the raw SH-4 closure.
- Reported ChuChu target `0x8C02CE0C`, reached from the nested dispatch at `0x8C0190A4`, is now discovered structurally; no manual seed or address whitelist is used.
- Targeted ChuChu validation: 4,980 reachable functions / 480,189 known SH-4 / 0 unknown / `RAW_SH4=0`; `0x8C02CE0C` is present in `function_map.csv`.
- The rule is bounded to aligned three-method rows with two already-proven siblings and an exact literal-fed tail-veneer shape.

## 0.0.204 (rebased from 0.0.193)

- Recover complete packed BRA selector-thunk families from an already-proven member: same local BRA worker, consecutive `MOV #selector,R4..R7` delay slots, minimum three members.
- ChuChu Rocket! runtime target `0x8C10A518` is recovered structurally as selector 6 of the seven-entry `0x8C10A500..0x8C10A518` family targeting `0x8C10A572`; no game-address whitelist is used.
- Recover compact literal-fed callbacks when a PC-relative literal is proven by local dataflow to reach an architectural `JSR/JMP`. A strict compact callable may override the generic pointer-density rejection because short leaf callbacks often place their literal pool immediately after `RTS`.
- The second live boundary `0x8C0186AE -> 0x8C04F698` is now recovered generically. `sub_8C04F698` is 4/4 known SH-4 instructions and requires no manual seed.
- Final ChuChu closure: **4,763 functions / 474,565 known SH-4 / 0 unknown / RAW_SH4=0**. `Direct literal targets` increases from 11 to 12 while the packed-BRA family count remains exactly 1, demonstrating bounded rather than explosive closure growth.
- Local commercial runtime passes the former missing target and reaches real TA/PVR activity: first captured checkpoint **7 renders / 6 flips**; the same bounded execution later reached **509 renders / 505 flips** without another unresolved SH-4 target.
- Generated session-log filenames and heartbeat `build=` labels are updated from the stale 0.0.200 diagnostic string to 0.0.204.
- Core and generated C++ regression: **53/53 PASS**.

## 0.0.203 (rebased from 0.0.193)

- SH-4 closure containment hotfix for the ChuChu Rocket regression exposed by 0.0.201/0.0.202.
- Prevent dirty/UNKNOWN speculative entries from contributing calls or table targets, and require a final decoder-clean admission gate for every new closure entry.
- Restrict the new ABI-argument bounded callback-table rule to 4-byte tables of 2..8 entries with strong callable targets; restore the older strict validation for memory-loaded bounded tables.
- Make global address-taken scanning a conservative safety net: >=4 clustered references, strong callable shape, and at most 128 promoted targets per scan.
- Return the commercial default budget to 8192 final functions / 6144 raw entries; closure progress is printed on every pass.
- CT2 regression remains clean at 3553 functions / 299380 known / 0 unknown / RAW_SH4=0; third-map target 0x8C03650C and A850/A8F0/A9F4 remain discovered.
- Core regression: 51/51 CTest PASS; CT2 runtime, runner, part_17, 0x8C03650C and bounded A850 table sources compile.

## 0.0.202 (rebased from 0.0.193)

- SH-4 closure: adds a conservative global address-taken proof pass for repeated, clustered in-image code pointers. Candidates are only promoted after callable-entry validation and a zero-UNKNOWN reachable CFG.
- CT2 compatibility: the third-map target `0x8C03650C` is now recovered generically from 16 static references and validates as 215/215 known SH-4 with 11 calls. The experimental package also registers the generated native fragment explicitly.
- SH-4 evidence: raw commercial recompilation writes `sh4_address_taken_evidence.csv` with candidate/proven state, reference density and decoded instruction/call counts.
- Commercial closure budgeting: separates raw synthetic-entry budget from final ProgramAnalysis budget. The default commercial BAT uses 12288 raw entries inside a 16384 final-function budget, preventing the old self-collision where filling `--max-functions` left no room for the authoritative CFG pass.
- Diagnostics: generated runtimes accept `--sh4-abi-audit`, which disables fast dynamic dispatch and verifies R8-R14/SP across ordinary indirect-call returns while excluding non-local/context-switch returns.
- CT2 PVR: retains the 0.0.202 modifier-volume/stencil path and frame-global PVR clamp register caching.
- Validation: core suite 51/51 passes; fresh CT2 raw closure reaches 3571 functions / 300703 known instructions / 0 unknown / 0 RAW_SH4; runtime and new callback translation units compile successfully.

## 0.0.201 (rebased from 0.0.193)

- SH-4 closure: extends conservative bounded callback-table discovery to selectors arriving in ABI argument registers R4-R7 and copied into the table-index register. Promotion still requires explicit non-negative and `< N` proof with `N=2..16`.
- SH-4 closure validation: bounded entries are accepted only after their complete decoded CFG has at least four instructions and zero unknown SH-4 opcodes; table-register clobbers between literal load and indexed read are rejected.
- CT2 compatibility: the real table at `0x0C111478` now promotes `0x8C06A850`, `0x8C06A8F0`, and `0x8C06A9F4`; all three decode cleanly and are packaged with 30 block-entry registrations.
- Generated-memory performance: CT2 uses a compile-time MMU-off specialization for the observed retail path so 32-bit main-SDRAM loads/stores execute inline in generated TUs, with exact fallback for non-SDRAM mappings and exact Store Queue handling.
- Generated stack/control stores use the same inline word path; ordinary `LOAD32`, FPU 32-bit loads, PR pops and control-register loads similarly avoid a cross-TU runtime call on main RAM.
- Keeps 0.0.200 FPSCR.DN correctness, host-sync quantum 524288, UI cadence 10 ms, profiler stride 67, TMU lazy path and SH-4 scheduler batch 256. No FPU precision or guest clock shortcuts were introduced.
- Validation: raw CT2 closure reaches 3548 functions / 298494 known instructions / 0 unknown / 0 RAW_SH4; core suite 51/51 passes; commercial CT2 `generated_compile_test` returns RC=0 and the native runner links successfully.

## 0.0.200 (rebased from 0.0.193)

- SH-4/FPU correctness: synchronizes FPSCR.DN with the host floating-point denormal mode whenever FPSCR is written through `LDS`, `LDS.L`, reset or runner override. This matches the Dreamcast rule used by Flycast: DN=1 flushes denormals to zero, DN=0 preserves them.
- Gameplay crash investigation: targets the historical `0x8C080CD8` fault seen with `fpscr=0x140000` (DN=1, SZ=1), where CT2 eventually reached an invalid geometry-stream pointer `0x58AC43B5`. The prior FSCHG/superblock-crossing hypothesis was checked and ruled out; `sfc_flush` already closed the superblock before the SZ transition.
- FPU cache hardening: cached/superblock fast paths may only remain active while their PR/SZ/RM eligibility still holds; an active cache no longer bypasses a changed FPU mode.
- Hot runtime cleanup: `sfc_tick`/`sfc_tick_hot` honor `DCR_DISABLE_HOT_TICK_CALL_COUNTER`, removing the diagnostic tick-call increment when that commercial option is enabled. Current CT2 mode 3 normally compiles the persistent SFC path out, so this is generic cleanup rather than a claimed CT2 speedup.
- Keeps 0.0.199 scheduler settings unchanged: SH-4 batch 256, host-sync quantum 524288, UI cadence 10 ms, profiler stride 67, TMU0 lazy path and lightweight current-PC tracking.
- Validation: main suite 51/51 passed; an x86 runtime smoke test verified that DN=0 preserves a subnormal while DN=1 flushes it to zero; selected CT2 commercial runtime/runner/hot shards/callback compile successfully.

## 0.0.199 (rebased from 0.0.193)

- Host UI: raises the guest-cycle gate from 1,000,000 to 2,000,000 SH-4 cycles and the wall-clock cap from 8 ms to 10 ms. The live window remains serviced at up to ~100 Hz while avoiding unnecessary `steady_clock`/`PeekMessage` work.
- Host AICA sync: replaces the old fixed 262,144-cycle mask crossing with an explicit re-armed deadline. CT2 defaults to 524,288 SH-4 cycles (~2.62 ms at 200 MHz).
- Any real host sync (including Present/sleep-driven syncs) re-arms the periodic deadline, preventing a redundant periodic sync immediately afterward.
- ARM7 catch-up budget scales with the host-sync quantum relative to the historical 262,144-cycle cadence, so doubling the interval does not halve the maximum executable firmware work per unit time.
- Heartbeat `host-sync-q` now reports the configured quantum instead of a hard-coded label.
- Keeps the 0.0.198 prime-stride profiler (67), lightweight current-PC tracking, 0.0.197 callback closure, TMU0 lazy path, PREF filtering, fixed SH-4 batch 256 and validated FPU mode.
- Validation: main suite 51/51 passed; CT2 `dc_runtime.cpp`, runner, hot geometry shard and `0x8C06C41C` callback compile with commercial definitions; dedicated host-sync deadline test passed.

## 0.0.197 (rebased from 0.0.193)
- Compatibility: adds the already-analyzed callback thunk `0x8C06C41C` to the CT2 AOT package. The analyzer closure from `0x8C06C35C` already discovers this target; the 0.0.196 failure was an incomplete manual supplemental-package registration, not a new decoder failure.
- Generated coverage: CT2 now registers the complete newly exposed local callback chain used by this path (`0x8C06C35C`, `0x8C06C41C`, `0x8C06C44A`, `0x8C06C4E6`, `0x8C06C71E`, `0x8C06C8EA`); the other closure targets are already present in the main AOT image.
- TMU performance: for the validated CT2 steady-state TMU0 mode (TPS=2, UNIE clear), exact SH-4 cycles are accumulated lazily and TCNT/UNF are materialized on MMIO read/write or before reconfiguration. Timer modes capable of interrupt delivery retain the existing eager path.
- PREF performance: generated guest `PREF @Rn` now returns inline for non-Store-Queue addresses; cached-RAM PREF is only a cache hint in this cacheless host model. Store Queue addresses still enter the exact `dc_pref` commit path.
- Keeps all 0.0.196 commercial specializations and the fixed 256-cycle SH-4 scheduler quantum. No guest CPU/PVR/AICA clock or IRQ cadence is accelerated.
- Validation: core suite 51/51 passed; dedicated TMU-lazy exactness test passed for normal decrements and UNF/reload; modified CT2 runtime, hot geometry shard, new callback thunk and runner compile successfully.

## 0.0.196 (rebased from 0.0.193)
- Performance lineage is 0.0.195_rebased193; guest SH-4 scheduling remains fixed at 256 cycles. No clock or IRQ cadence is accelerated.
- CT2 commercial build fixes the validated FPU mode (`regionplus`, mode 3) and disables FPU region telemetry at compile time so generated hot functions constant-fold unused mode branches. The generic recompiler remains runtime-selectable.
- SH-4 fast tick specializes the fixed 256-cycle commercial cadence and removes the purely diagnostic per-call counter from the ~1.75B-call hot path; full-tick accounting remains available.
- TMU0 gets an exact fast path for CT2's observed TPS=2 (core/256) case, preserving TCNT/UNF/reload semantics.
- Perf sampling replaces stride-64 modulo with a precomputed power-of-two mask while retaining the generic modulo fallback.
- Dynamic-call history is now opt-in through `DCR_TRACE_HISTORY=1`; the CT2 audit BAT enables it, while performance runs avoid writing a 32-entry ring on every successful dynamic call.
- CT2 performance builds use lightweight hot-hit metrics: ultra-hot successful dispatch/direct-call hit counters are compiled out, while misses/fallbacks remain measurable. This removes diagnostic writes from hundreds of millions of calls without changing guest behavior.
- Retains the 0.0.195 SH-4 closure fixes that completed the full menu attract/demo without `DCR SH4-FAULT` or unresolved target.

## 0.0.195 (rebased from 0.0.193)
- Compatibility lineage remains 0.0.193. All Crazy Taxi 2 launch scripts now keep `--sh4-tick-batch 256`; none of the discarded historical 512/4096 scheduler experiments are used for CT2 testing.
- SH-4 closure: recognizes exact `MOV.L literal,R0 ; RTS ; NOP` getters that return an executable function pointer. This recovers the observed callback `0x8C06C35C` generically.
- SH-4 closure: adds a conservative small-table rule that requires a proven non-negative bounded selector (`0 <= index < N`, N=2..16) before promoting 2/3-entry indexed callback tables. The observed `0x8C112970` table now promotes `0x8C06C44A` and `0x8C06C4E6`.
- CT2 generated coverage: adds the newly exposed local chain `0x8C06C35C`, `0x8C06C44A`, `0x8C06C4E6`, `0x8C06C71E`, and `0x8C06C8EA`; their resolved external dependencies already exist in the package.
- TMU performance: TPSC divisors 16/64/256/1024/4096 now use exact shifts/masks; the slow helper is entered only for an actual underflow/reload.
- Scheduler performance: skips render-done, TMU, idle GD-ROM/CDDA, and Holly IRQ helpers when their preconditions prove they cannot produce an event. Guest event ordering and clock rates are unchanged.
- Diagnostics performance: heartbeat host-clock polling is gated by accumulated guest cycles instead of querying `steady_clock` on every full scheduler tick.
- TA performance: staging records Type-7 and end-of-strip bits in the existing one-byte source metadata, letting bulk strip scans avoid re-reading each 32-byte packet PCW.
- Retains 0.0.194-rebased TA_ISP_CURRENT compact mirroring and Type-7 front-end fast path. No Dreamcast timing is accelerated to manufacture 60 FPS.

## 0.0.194 (rebased from 0.0.193)
- Discards the previous 0.0.194/0.0.195 scheduler experiments and uses 0.0.193 as the sole functional parent. SH-4 tick batching remains 256 cycles.
- PVR/TA: replaces the per-packet sparse-MMIO `unordered_map` lookup for read-only `TA_ISP_CURRENT` with an exact compact mirror while preserving guest readback.
- PVR/TA front-end: force-inlines packet-state consumption and returns immediately for the dominant 32-byte Type-7 vertex case, which is state-neutral.
- Adds a staging regression covering `TA_LIST_INIT -> TA_ISP_CURRENT -> +32-byte packet` readback. No IRQ/device-clock/GD-ROM ordering changes.

## 0.0.193
- CT2 wait-loop acceleration: the proven no-op callback `0x8C1560B0` now fast-forwards guest SH-4 time in bounded chunks while C44 is busy, stopping at render-done/VBlank/Holly boundaries instead of executing ~70k-150k host-side spin iterations per frame. Heartbeat adds `wait157-ff=hits/cycles/max_chunk`.
- CT2 asset recovery: the observed invalid table lookup in `0x8C075070` now dumps the last eight record hops and exits through the original guest function epilogue rather than remapping the invalid `0x87B1CA68` address or aborting the runner.
- SH-4 AOT closure: promoted four callable raw-boot targets missed by the supplemental CT2 closure (`0x8C033F62`, `0x8C03425E`, `0x8C03E700`, `0x8C06CD06`) plus the direct dependency `0x8C0343B6`; this fixes the observed `0x0C03E700` unresolved dynamic call.
- Added `tools/validate_supplemental_literal_closure.py`: callable `LOAD_LITERAL32 -> sub_*` sequences followed by dynamic call/branch must have a registered AOT target; pointer/data literals are not blindly promoted.
- CT2 asset diagnostics: the variable-record walker at `0x8C075000` keeps the last eight `record/index/step/next` hops and dumps them before an out-of-main-RAM table lookup, targeting the observed `0x87B1CA68` failure without changing guest behavior.
- TA profiling: removed the remaining `perf_profile_enabled` guard from the direct Type-7/8 strip path in both the CT2 runtime and generic emitter, so profiling no longer forces the slower per-vertex route.
- Retains the 0.0.191 MMU/UTLB + SH-4 register fault dump, 0.0.189 STARTRENDER completion fix, and 0.0.190 GPU inverse-W depth fix.
## 0.0.190
- GPU depth: preserve Dreamcast inverse-W ordering with pixel-shader `SV_Depth` mapping.
- Positive inverse-W > 1 no longer forces whole-frame CPU fallback.
- Retains 0.0.189 STARTRENDER completion fix.

## 0.0.190
- SPG_STATUS now advances from guest SH-4 cycles instead of register-read count.
- SPG line/frame timing follows SPG_LOAD, FB_R_CTRL.vclk_div and SPG_CONTROL.interlace.
- SPG_CONTROL/SPG_LOAD/vclk_div timing changes re-phase the raster and frame scheduler.
- PVR frame cadence now derives from guest SPG timing instead of the host window FPS target.
- Added heartbeat `spg=scanline/field/status/lineCycles/frameCycles/reads/resyncs`.
- Retains CT2 `wait157` telemetry and the 0.0.183 visual depth baseline.
