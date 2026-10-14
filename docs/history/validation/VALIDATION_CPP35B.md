# cpp35b validation

## ABI

- Guest ABI remains 1.
- `DCRuntime` struct body is byte-for-byte identical to cpp35a.
- SHA-256 of the struct body in cpp35a and cpp35b: `3957059438eb8a10051b2f40a047fe5998b9b610fd8ce22bfe32178695c22ecd`.
- `dcr_maple` contains no `compat_device_*` calls.

## Runtime/plugin build

Linux runtime build succeeded for:

- `DreamcastRuntime`
- `dcr_network`
- `dcr_maple`

Windows CMake target retains `user32` + `winmm` linkage for keyboard/WinMM/PS4 input; XInput is dynamically resolved as before.

## Private-state Maple smoke

With ABI2 Maple loaded:

- controller state set/read through the V2 control interface: PASS;
- controller DEVINFO DMA: PASS;
- Holly Maple DMA IRQ bit 12: PASS;
- VMU A1 DEVINFO DMA: PASS;
- private VMU image creation: PASS;
- VMU image size: 131072 bytes;
- log reports `private-state=yes`;
- Network remains cpp34s ABI1.

Observed VMU test path:

`/mnt/data/maple35b_system/dreamcast_vmu_a1.bin`

## Fallback smoke

With `dcr_maple` removed and Network left loaded:

- Maple plugin kind absent: expected;
- built-in cpp34 Maple MMIO read/write: PASS;
- built-in controller helper path: PASS.

## Regression suite

CTest: **54/54 PASS**.
