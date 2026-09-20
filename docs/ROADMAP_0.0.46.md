# DreamcastRecomp roadmap — after 0.0.46

## 0.0.46 — code checkpoint complete, retail acceptance pending

Implemented and locally regression-tested:

- generic dense absolute `JSR/JMP @Rn` callable-table discovery;
- conservative long-target validation using a clean 32-byte SH-4 prefix;
- raw symbol-free closure promotion of the discovered callable family;
- ARM7 reset-release RAM/vector snapshot;
- PC high-water and zero-opcode fall-through diagnosis;
- explicit no-wrap classification when fetch reaches `0x00200000`;
- 47/47 CTest and the exact 155-ELF KallistiOS regression remain clean;
- real standard KOS `sound/sfx` still produces non-zero native PCM with zero ARM7 faults.

The one missing acceptance input is the external retail ChuChu Rocket! CDI. The next Windows run must establish whether the old `0x8C138620` boundary is now in closure and provide the new ARM7 evidence.

## 0.0.47 — first priority

Use the 0.0.46 retail trace to continue only from demonstrated boundaries:

1. If `Dense dispatch targets` is non-zero and `0x8C138620` executes, keep the generic table rule and follow the next real unresolved dependency.
2. If the same table is still missed, inspect the exact data-flow feeding its index/base and generalize that pattern; do not add title-address seeds.
3. Classify the ARM7 `0x00200000` event from the release snapshot. Fix firmware loading/reset sequencing if the vectors/program are absent; investigate a documented mapping/control-flow rule only if the captured evidence requires it.
4. Continue toward the first stable commercial PVR frame with `unknown SH-4=0` and `RAW_SH4=0` preserved.

## Longer term

After a stable commercial frame, prioritize correct scene progression, Maple input, GD-ROM streaming and commercial AICA behavior before graphics polish. Keep the architecture as static/generated native code rather than drifting into a runtime SH-4 JIT/emulator fallback.
