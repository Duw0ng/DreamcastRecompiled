# v1.2 cpp38e — STABLE HOT_BLOCK_RECOVERY (2026-10-07)

- Promoted `cpp38e HOT_BLOCK_RECOVERY` to the official v1.2 stable development baseline.
- Keeps HOTPC profiling disabled by default for clean performance measurements.
- Preserves the validated hot-path/codegen recovery behavior used by the accepted ChuChu Rocket! run.
- Future `cpp38f+` work should branch from this baseline.

---

# cpp38b2 — FPU_METRICFREE_RECOVERY

- Fixes the cpp38b1 bug where Superblock implied `fpu-metrics=on` even with `DCR_FPU_METRICS=0`.
- Generated scalar/Region/Superblock FPU telemetry is now gated through metric macros.
- Modular production guest compiles with `DCR_FIXED_FPU_REGION_METRICS=0`, so MSVC/GCC can eliminate the telemetry updates from the hot path.
- Existing ABI1 caches migrate at generated-C++ level and rebuild once without re-running SH-4 closure discovery.
- Keeps cpp38b1 index scratch, Superblock and 262144 host sync unchanged for a clean A/B.
- Guest ABI1 structure remains frozen.

# cpp38b1 — PVR_SUPERBLOCK_RECOVERY

- Reverts cpp38b direct mapped-index construction after live Windows A/B showed lower FPS than cpp38a under almost identical PVR load.
- Restores cpp38a CPU `index_scratch` construction followed by one bulk D3D11 index upload.
- Makes static FPU `superblock` the runtime default again; `regionplus` remains selectable for A/B.
- Forces hot FPU metrics OFF by default; diagnostics can re-enable them explicitly.
- Keeps the 262144-cycle host/AICA sync cadence and cpp38a SQ/CH2/SIMD TA fast paths.
- Guest ABI1 and pinned SAFE ChuChu guest remain unchanged.
- Portable launcher v1.3 / short `[cpp38b1]` titles.

# cpp38b — PVR_GPU_BATCH_TIMING_RECOVERY

- Direct-mapped D3D11 R32 index stream: removes one full per-frame index scratch copy.
- Restores 262144-cycle host/AICA synchronization by runtime initialization, preserving frozen ABI1 layout.
- Adds direct 262144 vs 524288 A/B benchmark scripts.
- Keeps cpp38a SQ/CH2/SIMD TA fast paths and cpp37c GD-ROM.
- Portable launcher v1.2 / short `[cpp38b]` titles.

# cpp38a — PVR_TA_NATIVE_FASTPATH

- PVR-first performance phase after cpp37c GD-ROM completion.
- One-copy SQ→TA capture.
- CH2 main-RAM→TA bulk staging and main-RAM→VRAM bulk texture copy.
- SSE2 complete-strip tag scan and homogeneous-source Type-7/8 direct decode.
- Guest ABI1 and SAFE guest unchanged.
- Launcher v1.1 / short `[cpp38a]` titles.

# cpp37c — GDROM_CDDA_NATIVE

- Added host-only `dcr.gdrom.control.v3` while preserving frozen Guest ABI1.
- Moved CDDA PLAY_TRACKS / PLAY_SECTORS / PAUSE / RELEASE / STOP / SEEK state into `dcr_gdrom.dll`.
- Moved raw 2352-byte Red-Book sector reads, track/FAD resolution, byte-order analysis, Yellow-Book reconstruction and impulse-repair state into the GD-ROM plugin.
- Runtime keeps the 75 Hz scheduler and final AICA/host audio mix bridge; decoded PCM crosses the DLL boundary in 64-frame batches rather than per sample.
- When V3 is active, DreamcastRuntime no longer opens its own CDDA disc stream; media I/O stays plugin-owned.
- Heartbeat now reports `gd-native=phase3` plus `gd-cdda=...` counters/state.
- Portable launcher v1.0 and per-game windows use concise `[cpp37c]` names.
- Runtime/plugin build and phase-3 CDDA smoke PASS; effective regression suite 54/54 PASS.

# cpp37b — GDROM_COMMAND_DMA_NATIVE

- `dcr.gdrom.control.v2`: plugin-private GD command state while Guest ABI1 stays frozen.
- Multi-transfer state/cursor moved to `dcr_gdrom.dll`.
- G1/GD DMA bulk transfer writes directly through Host Services V2 and raises/acks the GD-DMA IRQ from the plugin.
- Heartbeat exposes `gd-native=phase2`, plugin media counters and DMA/PIO counters without verbose trace.
- Runtime build ID corrected to cpp37b.
- Normal game/PVR/status windows use short per-game titles: `DreamcastRecompiled - <game> [cpp37b]`.
- Portable launcher v0.9 starts as `DreamcastRecompiled Portable [cpp37b]` and changes to the probed game title after reading `disc.map`.
- CDDA remains intentionally in Runtime for cpp37c.

## cpp37a — GD-ROM Native Plugin Phase 1
- Added ABI2 `dcr_gdrom.dll` (plugin kind 4).
- Moved DiscMap V1/V2 parsing, physical track geometry, sessions, TOC and bulk logical sector reads out of DreamcastRuntime.
- Runtime keeps a compatibility geometry mirror for CDDA and command state not yet migrated.
- Added `--gdrom-trace`, `--rebuild-gdrom`, portable launcher v0.8 GD-ROM Trace and portable auto-sync of `dcr_gdrom.dll`.
- Guest ABI1 fingerprint and SAFE ChuChu guest are unchanged.
- GD-ROM plugin/bridge smokes PASS; effective regression suite 54/54 PASS.

## cpp36b1g — Sony Haptics + Dreamcast Visual V2
- Added native Sony HID rumble path for DualShock 4 and DualSense/PS5, USB and Bluetooth report layouts.
- PuruPuru host haptics now use XInput or Sony HID automatically for WinMM/DirectInput-selected Sony pads.
- Modern tester calls `dcr_maple.dll` directly for rumble testing and reports the detected haptics backend.
- Reworked controller preview toward the original Dreamcast silhouette and authentic A-red/B-blue/X-yellow/Y-green face colors.
- Added double-buffered visual painting and repaint-on-state-change to reduce flicker.
- Guest ABI1 and pinned SAFE ChuChu guest remain unchanged.

## cpp36b1f — Dreamcast Controller Visual
- Added a live full Dreamcast controller visualization to the modern per-port tester.
- Shows mapped Dreamcast controls, analog stick movement and analog trigger pressure.
- Launcher-only update; Runtime build ID remains cpp36b1e and Guest ABI1 is unchanged.

## cpp36b1e — RUNTIME_EXTERNAL_RAW_ENV

- Fixes `Unknown runner option: --external-raw-image` when adopting the portable-safe ChuChu Guest ABI1 module with an older cached `dreamcast_program.exe`.
- `run_game.bat` now passes the prepared bootstrap via `DCR_EXTERNAL_RAW_IMAGE` and `DCR_EXTERNAL_RAW_BASE` environment variables instead of requiring a new runner CLI.
- `DreamcastRuntime.dll` loads the external bootstrap before commercial boot staging and refreshes static code signatures after the bytes are present.
- Portable launcher v0.5 uses the same environment-variable path.
- Guest ABI1 and the SAFE `dcr_game_module.dll` remain unchanged.

> **cpp36b1d AUTO_PORTABLE_SYNC_FIX:** robust SAFE guest detection now marks the existing pinned ChuChu ABI1 DLL portable-safe even with padded title/volume metadata. `--reuse` creates/refreshes the portable automatically; `--sync-portable` does the same and exits without launching. No SH-4 rebuild.

# v1.2 cpp36b1a AUTO_PORTABLE_SAFE_SEED — 2026-10-05

- Bundles the user-supplied ChuChu online portable Guest ABI1 module as a SHA-256-pinned safe seed.
- Verifies ABI export `0x00010000` and current cpp34x Late-AOT target presence.
- `run_game.bat --reuse` automatically adopts the seed into an existing valid ChuChu ABI1 cache if that cache is still legacy/non-portable.
- Adoption performs no SH-4 generation or compilation and immediately enables cpp36b1 auto-portable synchronization.
- Adds `--no-safe-seed` opt-out and `TEST_CPP36B1A_ADOPT_SAFE_GUEST.bat`.

# v1.2 cpp36b1 AUTO_PORTABLE_GUEST — 2026-10-05

- Normal commercial SH-4 generation now uses `--no-embed-raw-image`, making the same `dcr_game_module.dll` usable by development and portable launchers.
- Generated runners accept `--external-raw-image`/`--external-raw-base` and load the user's locally prepared bootstrap before static signatures/commercial boot setup.
- `run_game.bat --reuse` auto-syncs a ready portable folder and ZIP whenever the cached guest is portable-safe; it never recompiles SH-4.
- Adds `--portableize` for the single migration of legacy guests that embedded `.raw_boot`.
- Guest ABI1 fingerprint remains unchanged.
- Full regression remains 54/54 PASS.

# v1.2 cpp36b PURUPURU_MODERN_INPUT — 2026-10-05

- Adds PuruPuru as a real Maple expansion device with DEVINFO/GETCOND/GETMINFO/BREAD/BWRITE/SETCOND/reset handling.
- Translates PuruPuru SETCOND to native XInput rumble with automatic stop timing.
- Port A defaults to VMU A1 in slot 1 + PuruPuru in slot 2.
- Adds independent A-D host input specs: `auto`, `none`, `keyboard_mouse`, `xinput:N`, `winmm:N`.
- Keyboard + Mouse is combined into one Dreamcast controller; physical controllers remain independently selectable.
- Preserves Maple Trace, Maple V3 device tree and cpp35b1 immutable Guest ABI cache.
- Guest ABI fingerprint unchanged; PuruPuru DMA smoke PASS; effective project suite 54/54 PASS.

# v1.2 cpp36a MAPLE_DEVICE_TREE — 2026-10-05

- Maple Device Tree V3: 4 controller ports + 2 expansion slots per port.
- Port-aware DMA and KOS/HLE enumeration/status paths.
- Port A keeps the existing profile path; B-D auto-map additional host devices.
- Adds `DCR_MAPLE_TRACE=1` / `--maple-trace`.
- Keeps cpp35b1 cache/ABI freeze; Guest ABI1 fingerprint unchanged.
- Validation: 4-port GETCOND smoke + IRQ PASS, effective 54/54 test suite PASS.

# v1.2 cpp35b1 CACHE_ABI_FREEZE — 2026-10-05

- Freezes Guest ABI v1 headers under `runtime/guest_abi/v1`.
- Replaces whole-file `dc_runtime.hpp` cache comparison with a canonical `DCRuntime` ABI fingerprint.
- Reuses compatible cpp34x/cpp35 caches even when runtime/plugin APIs outside `DCRuntime` change.
- Treats a valid guest cache as immutable: no modularizer and no guest CMake reconfiguration on runtime/plugin updates.
- Builds `DreamcastRuntime`, `dcr_network` and `dcr_maple` in a separate runtime CMake tree and writes the DLLs into the existing runner build directory.
- `--rebuild-runtime` no longer targets `dreamcast_program`/`dcr_game_module`.
- Adds `TEST_CPP35B1_NO_GUEST_REBUILD.bat` (`--reuse --rebuild-runtime`) to make accidental full SH-4 recompilation impossible during validation.
- Validation: source invariants PASS; cpp34x ABI fingerprint MATCH; runtime-only build leaves guest binary/object hash and mtime unchanged; full project suite 54/54 PASS.

# v1.2 cpp35b MAPLE_PRIVATE_STATE — 2026-10-05

- Moves Maple MMIO/DMA, controller state/profile polling and VMU A1 state/persistence into `dcr_maple` ABI2 private storage.
- Removes cpp35a `compat_device_*` bridge use from Maple.
- Routes KOS/HLE Maple helpers through a bound V2 control interface so low-level DMA and high-level calls share the same plugin state.
- Keeps Guest ABI1 / `DCRuntime` struct body unchanged and retains cpp34 built-in Maple fallback.
- Network remains cpp34s ABI1.
- VMU remains 128 KiB and persists under the system-data directory.
- Validation: private-state DMA/VMU smoke PASS, fallback PASS, **54/54 CTest PASS**.

# v1.2 cpp35a — PLUGIN_RUNTIME_V2_MAPLE_BRIDGE — 2026-10-05

- Adds **Plugin ABI V2** in parallel with frozen ABI1; no fields are added to `DCRuntime`.
- Adds `DCRHostServicesV2` with version/size, memory access, memory-region lookup, IRQ services, logging/config hooks and a transitional device-compat bridge.
- Plugin host now prefers `dcr_plugin_query_v2` when available and keeps `dcr_plugin_query_v1` fully supported.
- Replaces the Network-only TLS fast cache with a generic per-kind cache for hot device dispatch.
- Adds 32-bit ABI2 MMIO hooks and `query_interface` for future PVR/AICA/GD-ROM fast paths.
- Adds `dcr_maple.dll` as the first ABI2 device. In cpp35a it owns the ABI2 dispatch boundary and delegates proven Maple behavior through the compatibility bridge; full private-state extraction is the next safe step.
- Maple DLL missing/incompatible => automatic built-in cpp34 fallback.
- Network remains cpp34s V42/LAPM ABI1 unchanged.
- `run_game.bat` adds `--rebuild-maple`; normal `dreamcast_program` builds depend on both Network and Maple plugins.
- Validation: DCRuntime body hash unchanged from cpp34x, ABI1 header byte-identical, core suite 54/54 PASS, ABI2/fallback MMIO smoke PASS, Maple DEVINFO DMA + Holly IRQ smoke PASS in both paths.

# cpp34x — AUTO Late-AOT 0x8C036280

- ChuChu online: agrega `0x8C036280` tras seguir avanzando dentro del Lobby Server real.
- Introduce auto-promoción de nuevos missing targets desde logs recientes al relanzar el mismo BAT, evitando una nueva distribución por cada callback indirecto.
- No cambia cpp34s network, ABI1, emitter ni guest principal.

# DreamcastRecomp cpp34w

- ChuChu online: agrega Late-AOT `0x8C036160` tras avanzar dentro del Lobby Server real.
- Mantiene los cuatro targets anteriores y toda la pila cpp34s sin cambios.
- No recompila el guest principal.

# DreamcastRecomp cpp34v

- ChuChu online: añade Late-AOT target `0x8C036236` tras alcanzar el servidor de lobby real en TCP/9001.
- Conserva targets `0x8C036102`, `0x8C036118`, `0x8C036526`.
- Sin cambios de networking/runtime ABI/guest principal.


## v1.2 cpp34s - V42/LAPM RELIABILITY
- Full LAPM ARQ state: N(R), RR/RNR, REJ retransmit, TX window and XID limits.
- T403-style inactivity RR(P) while preserving cpp34r network fastpath.
- Rich V42 submit diagnostics; ABI1 and guest AOT unchanged.
# cpp34r — Network Fast Path + Submit Trace
- Removed per-MMIO network-state and plugin-host mutexes from the hot path via TLS caches.
- Throttled full modem diagnostics and host socket polling.
- Added bounded serial catch-up and connected-tick early return.
- Added START-triggered submit trace without changing ABI1 or guest code.

## v1.2 cpp34c — FLASH_BASE0_FIX_ABI1

- Fixed the pre-modem flash lookup loop by populating retail logical block 0 at `0x1C040`.
- Existing cpp34b flash images migrate in place; console ID and ISP records are preserved.
- Guest ABI remains v1; no SH-4 game recompilation is required from cpp34b.

# v1.2 cpp34b MODULAR_FULL_ISP_ABI1 — 2026-10-03

- Keep cpp34 Guest ABI v1 and modular `dcr_network` plugin architecture.
- Correct PlanetWeb flash profile using the documented 0x80-0x84 + C0-C5 layout; add missing 0x81/C3/C4/C5 records.
- Add DreamKey C6-C8 and generic Katana E0/E2/E4-E9/EB compatibility records in the same persistent profile.
- Preserve the existing persistent console ID while automatically migrating partition 2 from cpp33/cpp34a layouts.
- Add `[BIOS FLASH HOT]` diagnostics with guest BIOS caller addresses for repeated configuration loops.
- Full project build PASS; regression suite 54/54 PASS.
- Modular network-only rebuild validated: exactly plugin compile + link, zero generated SH-4 objects.

# v1.2 cpp34a MODULAR_RUNTIME_ABI1 — 2026-10-03

- Introduces **Guest ABI v1** so generated SH-4 is compiled as `dcr_game_module.dll` and can survive runtime/plugin updates.
- Moves runtime/device code into `DreamcastRuntime.dll`.
- Adds a generic optional device-plugin ABI with reserved kinds for Network, PVR, AICA, GD-ROM and Maple.
- Externalizes the cpp33c modem/PPP/UDP/DNS implementation into the first real device plugin: `dcr_network.dll`.
- Keeps the cpp33c built-in network code only as a safe fallback if the plugin is absent or ABI-incompatible.
- `run_game.bat` now uses a persistent guest cache under `%LOCALAPPDATA%\DreamcastRecompiled\games` by default.
- Adds `--recompile-game`, `--rebuild-runtime`, and `--rebuild-network`.
- The modularizer uses copy-if-different for the ABI header, preventing runtime-only releases from invalidating thousands of generated guest translation units.
- Validation: original core suite **54/54 PASS**; modular generated fixture builds and runs; `dcr_network` is dynamically loaded; touching only `dcr_network.cpp` recompiles exactly the plugin object + DLL and **zero guest SH-4 objects**.
- Removes the runtime source include path from the guest target interface, so extracting a new ABI-compatible release into a different directory no longer invalidates generated SH-4 object files.
- Moves persistent Dreamcast flash/VMU storage to `%LOCALAPPDATA%\DreamcastRecompiled\system` by default, with optional one-time import through `DCR_IMPORT_SYSTEM_DIR`.

## v1.2 cpp33c — PLANETWEB_DUAL_ISP
- Add PlanetWeb 0x80/0x82/0x83/0x84 plus C0/C1/C2 ISP records for NTSC-U browser-compatible titles.
- Retain DreamKey C6/C7/C8 in the same Katana flash partition.
- Use standard phone `111-1111`, user `dream`, password `dreamcast`, DNS `46.101.91.123` / `1.1.1.1`.
- Migrate old cpp33a/cpp33b ISP partitions while preserving console ID.

# v1.2 cpp33c PLANETWEB_DUAL_ISP — 2026-10-02

- Forked from frozen v1.2 Stable.
- Persistent flash + DreamKey PPP/DNS.
- Native modem + virtual ISP carrier.
- PPP LCP/PAP/IPCP + Windows UDP/DNS bridge.
- TCP detection prepared for cpp33c.

# v1.2 Stable — frozen 2026-10-02

- Frozen from cpp32a CPP23 Callback Closure Rebase after ChuChu Rocket stopped hitting the known SH-4 missing-target crashes.
- No runtime behavior changes in this release; version/identity only.
- Network development moved to cpp33_NETWORK.

# cpp32a CPP23 callback-closure rebase
- Rebases the callback/state-table closure fix on **cpp23_GAME_PROFILES_REGRESSION_FIX**, not on the older main snapshot.
- Preserves cpp23 game profiles, renderer/audio/runtime behavior and all later callback validators.
- Adds conservative two-level Katana state-table recovery: 3-method rows, >=2 known methods per anchor row, maximum 2 strict bridge rows, duplicate-row aliases only after an accepted row.
- Every promoted target is revalidated with `analyze_code_fragment_at` and must decode with zero unknown SH-4 instructions.
- Adds `Nested state callbacks` telemetry to `dc_raw_recomp`.
- Motivated by ChuChu indirect dispatcher `0x8C0190A4` reaching missing targets `0x8C04FFC0` and `0x8C034FA0`; no title-specific address seeds are added.

# cpp23 game-profile regression repair
- Adds persistent per-game `game.cfg` files and isolated generated/build workspaces.
- Restores pre-cpp11 plain commercial staging for Daytona/Lodoss while keeping CT2/SGGG scrambled and ChuChu plain.
- Makes boot stage, renderer, seeds, entry path, closure limits, audio/clock policy and extra runtime args profile-driven.
- `--reuse` and `--clean` are now scoped to the detected title.

# Native GPU PVR checkpoint (cpp20)

- Moves Dreamcast texture decoding for the D3D11 fast path to a feature-level-11 compute shader, retaining CPU decode only as a compatibility/fallback path.
- Removes the per-frame `PVRSoftVertex -> PVRGpuVertex` conversion and uploads the TA vertex arena directly.
- Replaces the single streaming VB/IB pair with a persistent three-slot ring that grows only when required.
- Keeps compatible PVR render-to-texture results GPU-resident and performs Dreamcast VRAM readback lazily only when CPU/SH-4 access requires it.
- Adds a compact PVR pipeline cache for rasterizer/blend/depth/sampler state bundles.
- Expands the GPU fast path so raw GPU-decode textures and negative inverse-W geometry no longer force avoidable whole-frame software replay.
- Introduces `PVRRendererBackend` and `PVRD3D11RendererBackend`, separating PVR/TA translation from the host renderer implementation before future Vulkan work.
- Adds `pvr-gnative=` heartbeat telemetry for compute decode, avoided CPU decode, pipeline-cache efficiency, GPU RTT residency and real fallback materialization.
- Validation: 52/52 normal tests + generated single/program builds PASS (54/54 logical tests). Actual D3D11 runtime validation remains a Windows-side test.

# Renderer-selection checkpoint (cpp19)

- Extends the canonical `run_game.bat` with a named renderer backend selector.
- Adds `--renderer=dx11`, `--renderer dx11`, `--gpu` and `--gpu=dx11` for the existing Direct3D 11 GPU PVR path.
- Keeps `--software` as a backward-compatible alias for `--renderer=software`.
- Adds `auto` mode so existing title defaults remain unchanged: Generic/CT2 use DX11; Daytona/Lodoss keep software unless explicitly overridden.
- Reserves `vulkan`, `dx12`, `dx10` and `dx9` names for future backends and reports them as not implemented instead of silently selecting DX11.
- Explicit renderer selection now applies consistently across Generic, CT2, Daytona and Lodoss runner paths.

# SGGG cpp11 recovery checkpoint

- Restores title-agnostic `.raw_boot` commercial scramble restaging using generated section metadata.
- Adds BIOS FlashROM INFO/READ HLE and keeps the flash syscall vector at `0x8C001004`.
- Adds `profiles/sggg_known_seeds.txt` for the six runtime-discovered SH-4 targets through cpp10.
- Moves the opt-in low-level Maple START diagnostic to the late PVR-frame 1800..5000 window for long-intro titles such as SGGG.
- Normal/manual controller behavior is unchanged unless the probe flag is explicitly supplied.

# DreamcastRecomp v0.1.1 Official — Crazy Taxi 2 compatibility recovery

- Restores the commercial Katana boot path for Crazy Taxi 2; the universal launcher no longer uses `--direct-game-entry=0x8C010000` for CT2.
- Re-bases the raw SH-4 closure logic on the last safe 0.0.210 analyzer and adds generic GPR ABI recovery for functions packed immediately after `RTS` delay slots (validated at `0x8C16BFB6`).
- Expands `profiles/crazy_taxi_2_known_seeds.txt` to the runtime-proven closure roots recovered from the 0.0.171–0.0.203 gameplay branch. The game code is still generated from the user's own CDI; no commercial game image or generated game code is shipped.
- Adds opt-in `--ct2-compat`, used by the CT2 launcher profile to reconstruct the skipped Shinobi `syMallocInit` postcondition when needed.
- Adds a low-level Maple CT2 regression autopilot behind the existing probe flag; it is disabled in normal play.
- Preserves v0.1 controller-profile, XInput and native PS4/DirectInput support.
- Final clean validation: CDI -> analysis -> 4,043 functions / 351,385 known SH-4 instructions / RAW_SH4=0 -> compile -> commercial boot -> VMU/title/menu -> real 3D gameplay. The last full heartbeat before the intentional 10,000,000-packet stop recorded 9,959,626 PVR packets, 2,670 frames and 300 GD-ROM requests with no missing-target or SH-4 fault.

# v0.1 SH-4 adjacent-entry hotfix (2026-09-20)

- Fixes symbol-free commercial closure missing a GPR ABI function that begins immediately after the previous function's `RTS` delay slot.
- The existing boundary comment promised GPR/FPU recognition, but the implementation only promoted FPU-only prologues at the exact adjacent boundary.
- Adds exact-boundary promotion for strong GPR ABI prologues (`R8-R14` save + `PR` save) without widening arbitrary raw-data scans.
- Crazy Taxi 2 failure addressed: dynamic target `0x8C16BFB6`, with nearest native entry `0x8C16BFAA` (`delta=0xC`).
- Synthetic regression reproduction: before patch `1` synthetic entry; after patch `2`, including `sub_8C16BFB6`; `RAW_SH4=0`.
- Full project validation remains green; the final generated-program build test also passes.

---

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

## cpp22 — GPU texture-cache hotfix

- Reuses immutable raw texture snapshots while the texture's own VRAM pages remain unchanged.
- Reuses palette snapshots by palette epoch/format.
- Replaces the 512-entry D3D11 texture-cache clear-all policy with single-entry LRU eviction.
- Adds `pvr-gcache=rawHits/rawMisses/palHits/palMisses/lruEvictions` telemetry.
- Adds `run_game.bat --reuse` / `--no-recompile` so an existing generated commercial build can launch without regenerating or recompiling SH-4 closure shards.
- Adds `apply_cpp22_texture_cache_hotfix.bat` to replace only the already-generated `dc_runtime.cpp` and incrementally relink the existing runner.

## cpp34t

- ChuChu online submit crossed V.42/PPP/IP/TCP in cpp34s and exposed the next native target `0x8C036118`.
- Late-AOT builder now accepts multiple `--target` arguments and registers `0x8C036102` + `0x8C036118` together.
- Keeps cpp34s network fast path/LAPM behavior and ABI1 unchanged; no 5185-function guest rebuild.

## cpp36b1c — MSVC Maple Input Fix

- Fixes MSVC 14.51 `std::clamp` ambiguity between Win32 `LONG` mouse coordinates and `int` limits.
- Guards Windows preprocessor macros to avoid redefinition warnings.
- Guest ABI1 and the adopted portable-safe ChuChu guest remain unchanged.
- This update rebuilds only Runtime/plugins; it does not recompile SH-4.
