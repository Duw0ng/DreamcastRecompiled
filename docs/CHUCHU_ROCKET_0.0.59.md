# ChuChu Rocket! checkpoint - 0.0.59

0.0.59 fixes the early-boot Maple regression introduced by 0.0.58.

When A1 is advertised, the Katana Maple stack may emit DMA response-buffer addresses with only the low 24 bits populated (for example 0x00000700). Maple DMA targets SH-4 area-3 SDRAM; normalize these descriptors to 0x0Cxxxxxx and 32-byte alignment at the Maple DMA boundary. Do not make Boot ROM/low physical memory writable.

VMU A1 remains enabled and persistent. Heartbeat field `maple-recv-fix` counts normalized DMA response destinations.

Commercial closure with ChuChu Rocket!: 2294 functions, 171568 known SH-4 instructions, 0 unknown, RAW_SH4=0.
