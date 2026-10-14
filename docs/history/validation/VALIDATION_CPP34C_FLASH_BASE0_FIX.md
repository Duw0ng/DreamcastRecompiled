# Validation — cpp34c FLASH_BASE0_FIX_ABI1

- Base: cpp34b MODULAR_FULL_ISP_ABI1 / Guest ABI v1.
- Public runtime/plugin ABI headers: unchanged from cpp34b.
- Full Linux build: PASS (`dc_raw_recomp` included).
- CTest: 54/54 PASS (the final generated runtime test was rerun separately after the container time slice expired).
- `generated_program_build`: PASS.
- Flash fix: physical partition-2 block `0x1C040` now contains CRC-valid logical block `0x0000` and bitmap bit 0 remains allocated.
- Existing persistent cpp34b flash images are migrated in place without changing the console ID or ISP/browser blocks.
- Guest SH-4 recompilation is not required when upgrading from cpp34b ABI v1; rebuild the runtime only.
