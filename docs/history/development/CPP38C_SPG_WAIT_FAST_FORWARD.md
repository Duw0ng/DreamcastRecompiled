# cpp38c — SPG WAIT FAST-FORWARD

cpp38c is a strict runtime-only SPG experiment on the cpp38 line. Guest ABI v1 remains unchanged. When an ABI1 cache already exists, cpp38c preserves its generated source and `dcr_game_module.dll` exactly as-is: no cpp38b2 metric-free migration, no generated-C++ CMake configure/build, no SH-4 discovery, and no guest relink. This keeps the A/B focused on the runtime SPG change.

The optimization targets the regression visible in cpp38b1/cpp38b2 where a stable scene can perform millions of PVR MMIO reads, almost all of them `SPG_STATUS` polls. `SPG_STATUS` remains derived from guest SH-4 cycle time; reads never advance the raster directly.

When the runtime sees the same guest PC read the same `SPG_STATUS` value repeatedly within <=256 guest cycles, it treats the sequence as a raster busy-wait. After four repeated reads it adds only the remaining cycles of the current scanline to the pending SH-4 scheduler tick. The next generated block enters the normal scheduler.

Safety bounds:
- disabled whenever a Holly interrupt is already pending;
- never stacks a second skip before the first reaches the scheduler;
- never crosses pending render-done;
- never crosses the next scheduled PVR frame/VBlank boundary;
- never crosses the host-sync boundary;
- one skip is at most one scanline and at most 0x40000 SH-4 cycles.

Control: `DCR_SPG_WAIT_FF=1` (default in cpp38c launchers) or `0` to disable. `run_game.bat` also accepts `--spg-ff-on` / `--spg-ff-off`.

Heartbeat: `spg-ff=on|off/HITS/CYCLES/MAX_CHUNK/0xLAST_PC`.


## Strict `--reuse` guarantee

`TEST_CPP38C_FAST.bat` passes `--reuse`. In this mode the launcher:
- does not build `dc_raw_recomp`;
- does not patch `generated/cpp`;
- does not configure the guest CMake tree;
- does not build `dreamcast_program`/`dcr_game_module.dll`;
- may build **DreamcastRuntime/plugins only**, once, because SPG_WAIT_FAST_FORWARD is implemented there.

If a valid ABI1 guest cache does not exist, `--reuse` aborts instead of compiling one.
