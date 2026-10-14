# DreamcastRecomp v1.2 cpp35b MAPLE_PRIVATE_STATE

- Completes Maple private-state extraction behind Plugin Runtime V2.
- `dcr_maple` owns controller, VMU, DMA/MMIO and persistence state.
- Removes all use of the cpp35a Maple compatibility read/write bridge from the plugin.
- Keeps frozen `DCRuntime`/Guest ABI1 layout unchanged.
- Adds a Maple V2 control interface so high-level KOS/HLE paths use plugin-owned state.
- Retains the cpp34 built-in Maple path as a safe fallback if the ABI2 plugin is missing.
- Keeps cpp34s Network ABI1 unchanged.
- VMU A1 remains a 128 KiB persistent `dreamcast_vmu_a1.bin` in the system-data directory.
- Validation: private DMA + VMU smoke PASS, fallback PASS, CTest 54/54 PASS.
