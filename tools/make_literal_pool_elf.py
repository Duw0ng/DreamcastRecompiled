#!/usr/bin/env python3
"""Generate a tiny ELF32/SH file with a GCC-style literal pool.

It models the pattern observed in KallistiOS hello.elf::_main:
  MOV.L @(disp,PC),R0 -> function pointer
  MOV.L @(disp,PC),R4 -> .rodata pointer
  JSR @R0
  RTS
  alignment NOP
  embedded DATA32 literals
"""
from __future__ import annotations

import argparse
import struct
from pathlib import Path

BASE = 0x8C010000
PRINTF = BASE + 0x40
RODATA = 0x8C020000
EM_SH = 42
ET_EXEC = 2
EV_CURRENT = 1


def align(value: int, alignment: int) -> int:
    return (value + alignment - 1) & ~(alignment - 1)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("output", nargs="?", default="samples/sh4_literal_pool.elf")
    args = ap.parse_args()
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)

    main_words = [
        0xD004,  # mov.l @(16,pc),r0 -> BASE+0x14
        0x4F22,  # sts.l pr,@-r15
        0xD404,  # mov.l @(16,pc),r4 -> BASE+0x18
        0x400B,  # jsr @r0
        0x0009,  # delay slot
        0xE000,  # mov #0,r0
        0x4F26,  # lds.l @r15+,pr
        0x000B,  # rts
        0x0009,  # delay slot
        0x0009,  # unreachable alignment padding
    ]
    main_code = b"".join(struct.pack("<H", w) for w in main_words)
    main_code += struct.pack("<I", PRINTF)
    main_code += struct.pack("<I", RODATA)
    assert len(main_code) == 28

    text = bytearray(0x44)
    text[: len(main_code)] = main_code
    for off in range(len(main_code), 0x40, 2):
        text[off : off + 2] = struct.pack("<H", 0x0009)
    text[0x40:0x44] = struct.pack("<HH", 0x000B, 0x0009)  # _printf stub

    rodata = b"Hello from DreamcastRecomp literal-pool test!\0"

    shstr = b"\0.text\0.rodata\0.shstrtab\0.strtab\0.symtab\0"
    sh_name = {name: shstr.index(name.encode()) for name in [".text", ".rodata", ".shstrtab", ".strtab", ".symtab"]}
    strtab = b"\0_main\0_printf\0hello_string\0"

    # ELF32 symbol: name, value, size, info, other, shndx
    sym_null = b"\0" * 16
    sym_main = struct.pack("<IIIBBH", strtab.index(b"_main"), BASE, 28, 0x12, 0, 1)
    sym_printf = struct.pack("<IIIBBH", strtab.index(b"_printf"), PRINTF, 4, 0x12, 0, 1)
    sym_string = struct.pack("<IIIBBH", strtab.index(b"hello_string"), RODATA, len(rodata), 0x11, 0, 2)
    symtab = sym_null + sym_main + sym_printf + sym_string

    ehsize = 52
    shentsize = 40
    text_off = 0x100
    rodata_off = align(text_off + len(text), 4)
    shstr_off = align(rodata_off + len(rodata), 4)
    strtab_off = shstr_off + len(shstr)
    symtab_off = align(strtab_off + len(strtab), 4)
    shoff = align(symtab_off + len(symtab), 4)

    shnum = 6
    shstrndx = 3

    ident = bytearray(16)
    ident[0:4] = b"\x7fELF"
    ident[4] = 1
    ident[5] = 1
    ident[6] = 1

    ehdr = struct.pack(
        "<16sHHIIIIIHHHHHH",
        bytes(ident), ET_EXEC, EM_SH, EV_CURRENT, BASE,
        0, shoff, 0, ehsize, 0, 0, shentsize, shnum, shstrndx,
    )

    def sh(name: int, typ: int, flags: int, addr: int, off: int, size: int,
           link: int = 0, info: int = 0, addralign: int = 1, entsize: int = 0) -> bytes:
        return struct.pack("<IIIIIIIIII", name, typ, flags, addr, off, size,
                           link, info, addralign, entsize)

    sections = [
        sh(0, 0, 0, 0, 0, 0),
        sh(sh_name[".text"], 1, 0x6, BASE, text_off, len(text), addralign=2),
        sh(sh_name[".rodata"], 1, 0x2, RODATA, rodata_off, len(rodata), addralign=4),
        sh(sh_name[".shstrtab"], 3, 0, 0, shstr_off, len(shstr)),
        sh(sh_name[".strtab"], 3, 0, 0, strtab_off, len(strtab)),
        sh(sh_name[".symtab"], 2, 0, 0, symtab_off, len(symtab), link=4, info=1, addralign=4, entsize=16),
    ]

    total = shoff + shnum * shentsize
    data = bytearray(total)
    data[:len(ehdr)] = ehdr
    data[text_off:text_off + len(text)] = text
    data[rodata_off:rodata_off + len(rodata)] = rodata
    data[shstr_off:shstr_off + len(shstr)] = shstr
    data[strtab_off:strtab_off + len(strtab)] = strtab
    data[symtab_off:symtab_off + len(symtab)] = symtab
    for i, header in enumerate(sections):
        begin = shoff + i * shentsize
        data[begin:begin + shentsize] = header

    output.write_bytes(data)
    print(f"Wrote {output} ({len(data)} bytes)")


if __name__ == "__main__":
    main()
