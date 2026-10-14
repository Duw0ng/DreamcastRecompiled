# DreamcastRecomp v1.2 cpp34c — FLASH_BASE0_FIX_ABI1

Built on cpp34b Guest ABI v1. No SH-4 guest ABI change.

## Fix

ChuChu Rocket cpp34b repeatedly executed the flash block lookup before the modem, with hot callers `0x8C10F234` (INFO) and `0x8C10EF2C` (READ), and repeatedly read physical block `0x1C040`. cpp34b marked bitmap user-block 0 as allocated but left that physical block erased (`0xFFFF`). Real retail Dreamcast flash images contain a CRC-valid logical block `0x0000` there.

cpp34c now:

- seeds a retail-compatible logical block 0 at `0x1C040`;
- keeps bitmap bit 0 consistent with that allocated block;
- auto-migrates an existing cpp34b persistent flash without changing console ID or browser/ISP records;
- reports `base0=ok` in the flash layout diagnostic;
- keeps Guest ABI v1 unchanged, so an existing cpp34b ChuChu game module is reusable.

The network plugin itself is functionally unchanged from cpp34b; only its build tag is bumped.
