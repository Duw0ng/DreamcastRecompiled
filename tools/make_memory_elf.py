#!/usr/bin/env python3
"""Generate a small ELF32/SH program for DreamcastRecomp 0.0.15 memory tests.

The program keeps five 32-bit integers in .data and a scratch word in .bss.

    _main:
        r4 = &_values
        r3 = &_scratch
        r1 = 5
        r0 = _sum_array(r4, r1)
        *r3 = r0
        r0 = *r3
        return r0

    _sum_array:
        r0 = 0
        do {
            r2 = *r4++
            r0 += r2
        } while (--r1 != 0)
        return r0

Expected native result: R0 == 150 and _scratch == 150.
This exercises PC-relative pointer literals, .data/.bss image loading, MOV.L
register-indirect loads/stores, MOV.L post-increment, pointer arithmetic, DT/BF,
and a recompiled function call.
"""
from __future__ import annotations

import argparse
import struct
from pathlib import Path

TEXT_BASE = 0x8C010000
MAIN = TEXT_BASE
SUM_ARRAY = TEXT_BASE + 0x40
DATA_BASE = 0x8C020000
VALUES = DATA_BASE
BSS_BASE = 0x8C030000
SCRATCH = BSS_BASE
EM_SH = 42
ET_EXEC = 2
EV_CURRENT = 1


def align(value: int, alignment: int) -> int:
    return (value + alignment - 1) & ~(alignment - 1)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("output", nargs="?", default="samples/sh4_memory.elf")
    args = ap.parse_args()
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)

    # _main @ 0x8C010000. The two final DATA32 values are literal-pool pointers.
    # D405 @ PC+0 resolves to +0x18, D306 @ PC+2 resolves to +0x1C.
    # BSR at +0x08 reaches _sum_array at +0x40: disp=(0x40-0x0C)/2=0x1A.
    main_words = [
        0xD405,  # +0x00 mov.l @(20,pc),r4 -> &_values literal @ +0x18
        0xD306,  # +0x02 mov.l @(24,pc),r3 -> &_scratch literal @ +0x1C
        0xE105,  # +0x04 mov #5,r1
        0x4F22,  # +0x06 sts.l pr,@-r15
        0xB01A,  # +0x08 bsr _sum_array
        0x0009,  # +0x0A delay slot
        0x2302,  # +0x0C mov.l r0,@r3       store sum to .bss
        0x6032,  # +0x0E mov.l @r3,r0       load it back
        0x4F26,  # +0x10 lds.l @r15+,pr
        0x000B,  # +0x12 rts
        0x0009,  # +0x14 delay slot
        0x0009,  # +0x16 alignment / unreachable padding
    ]
    main_code = (
        b"".join(struct.pack("<H", w) for w in main_words)
        + struct.pack("<I", VALUES)
        + struct.pack("<I", SCRATCH)
    )
    assert len(main_code) == 0x20

    # _sum_array @ +0x40.
    # BF at +0x48 loops back to +0x42: disp=(0x42-(0x48+4))/2=-5=0xFB.
    sum_words = [
        0xE000,  # +0x00 mov #0,r0
        0x6246,  # +0x02 mov.l @r4+,r2
        0x302C,  # +0x04 add r2,r0
        0x4110,  # +0x06 dt r1
        0x8BFB,  # +0x08 bf +0x02
        0x000B,  # +0x0A rts
        0x0009,  # +0x0C delay slot
    ]
    sum_code = b"".join(struct.pack("<H", w) for w in sum_words)
    assert len(sum_code) == 14

    text = bytearray(0x40 + len(sum_code))
    text[:len(main_code)] = main_code
    for off in range(len(main_code), 0x40, 2):
        text[off:off+2] = struct.pack("<H", 0x0009)
    text[0x40:0x40+len(sum_code)] = sum_code

    values = [10, 20, 30, 40, 50]
    data_section = b"".join(struct.pack("<I", v) for v in values)
    bss_size = 4

    shstr = b"\0.text\0.data\0.bss\0.shstrtab\0.strtab\0.symtab\0"
    names = [".text", ".data", ".bss", ".shstrtab", ".strtab", ".symtab"]
    sh_name = {name: shstr.index(name.encode()) for name in names}
    strtab = b"\0_main\0_sum_array\0_values\0_scratch\0"

    # Section indices: 0 null, 1 .text, 2 .data, 3 .bss, 4 shstr, 5 strtab, 6 symtab.
    sym_null = b"\0" * 16
    sym_main = struct.pack("<IIIBBH", strtab.index(b"_main"), MAIN, len(main_code), 0x12, 0, 1)
    sym_sum = struct.pack("<IIIBBH", strtab.index(b"_sum_array"), SUM_ARRAY, len(sum_code), 0x12, 0, 1)
    sym_values = struct.pack("<IIIBBH", strtab.index(b"_values"), VALUES, len(data_section), 0x11, 0, 2)
    sym_scratch = struct.pack("<IIIBBH", strtab.index(b"_scratch"), SCRATCH, bss_size, 0x11, 0, 3)
    symtab = sym_null + sym_main + sym_sum + sym_values + sym_scratch

    ehsize = 52
    shentsize = 40
    text_off = 0x100
    data_off = align(text_off + len(text), 4)
    bss_off = data_off + len(data_section)  # SHT_NOBITS consumes no file bytes.
    shstr_off = bss_off
    strtab_off = shstr_off + len(shstr)
    symtab_off = align(strtab_off + len(strtab), 4)
    shoff = align(symtab_off + len(symtab), 4)
    shnum = 7
    shstrndx = 4

    ident = bytearray(16)
    ident[0:4] = b"\x7fELF"
    ident[4] = 1  # ELFCLASS32
    ident[5] = 1  # little endian
    ident[6] = 1  # EV_CURRENT
    ehdr = struct.pack(
        "<16sHHIIIIIHHHHHH", bytes(ident), ET_EXEC, EM_SH, EV_CURRENT, MAIN,
        0, shoff, 0, ehsize, 0, 0, shentsize, shnum, shstrndx,
    )

    def sh(name: int, typ: int, flags: int, addr: int, off: int, size: int,
           link: int = 0, info: int = 0, addralign: int = 1, entsize: int = 0) -> bytes:
        return struct.pack("<IIIIIIIIII", name, typ, flags, addr, off, size,
                           link, info, addralign, entsize)

    sections = [
        sh(0, 0, 0, 0, 0, 0),
        sh(sh_name[".text"], 1, 0x6, TEXT_BASE, text_off, len(text), addralign=2),
        sh(sh_name[".data"], 1, 0x3, DATA_BASE, data_off, len(data_section), addralign=4),
        sh(sh_name[".bss"], 8, 0x3, BSS_BASE, bss_off, bss_size, addralign=4),
        sh(sh_name[".shstrtab"], 3, 0, 0, shstr_off, len(shstr)),
        sh(sh_name[".strtab"], 3, 0, 0, strtab_off, len(strtab)),
        sh(sh_name[".symtab"], 2, 0, 0, symtab_off, len(symtab), link=5, info=1, addralign=4, entsize=16),
    ]

    total = shoff + shnum * shentsize
    blob = bytearray(total)
    blob[:len(ehdr)] = ehdr
    blob[text_off:text_off+len(text)] = text
    blob[data_off:data_off+len(data_section)] = data_section
    blob[shstr_off:shstr_off+len(shstr)] = shstr
    blob[strtab_off:strtab_off+len(strtab)] = strtab
    blob[symtab_off:symtab_off+len(symtab)] = symtab
    for i, header in enumerate(sections):
        begin = shoff + i * shentsize
        blob[begin:begin+shentsize] = header

    output.write_bytes(blob)
    print(f"Wrote {output} ({len(blob)} bytes)")
    print("Expected native result: R0=150 and _scratch=150")


if __name__ == "__main__":
    main()
