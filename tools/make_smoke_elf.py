#!/usr/bin/env python3
"""Generate a tiny legal ELF32/SH file for DreamcastRecomp decoder smoke tests.

This is not a KallistiOS binary and does not require the Dreamcast SDK. It only
contains a small SH-4 instruction stream plus ELF symbols/sections.
"""
from __future__ import annotations

import argparse
import struct
from pathlib import Path

BASE = 0x8C010000
EM_SH = 42
ET_EXEC = 2
EV_CURRENT = 1


def align(value: int, alignment: int) -> int:
    return (value + alignment - 1) & ~(alignment - 1)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("output", nargs="?", default="samples/sh4_smoke.elf")
    args = ap.parse_args()
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)

    # A GCC-like function prologue/body/epilogue. The PC-relative MOV.L loads
    # the value at BASE+0x20, where .rodata is mapped.
    words = [
        0x2FE6,  # mov.l r14,@-r15
        0x4F22,  # sts.l pr,@-r15
        0x6EF3,  # mov r15,r14
        0xE10A,  # mov #10,r1
        0xE214,  # mov #20,r2
        0x312C,  # add r2,r1
        0xD304,  # mov.l @(16,pc),r3 -> 0x8C010020
        0x430B,  # jsr @r3
        0x0009,  # nop (delay slot)
        0x4F26,  # lds.l @r15+,pr
        0x6EF6,  # mov.l @r15+,r14
        0x000B,  # rts
        0x0009,  # nop (delay slot)
    ]
    text = b"".join(struct.pack("<H", w) for w in words)
    rodata = struct.pack("<I", 0x8C020000)

    shstr = b"\0.text\0.rodata\0.shstrtab\0.strtab\0.symtab\0"
    sh_name = {
        ".text": shstr.index(b".text"),
        ".rodata": shstr.index(b".rodata"),
        ".shstrtab": shstr.index(b".shstrtab"),
        ".strtab": shstr.index(b".strtab"),
        ".symtab": shstr.index(b".symtab"),
    }
    strtab = b"\0_start\0main\0"
    start_name = strtab.index(b"_start")
    main_name = strtab.index(b"main")

    # ELF32 symbol: name, value, size, info, other, shndx
    sym_null = b"\0" * 16
    sym_start = struct.pack("<IIIBBH", start_name, BASE, len(text), 0x12, 0, 1)
    sym_main = struct.pack("<IIIBBH", main_name, BASE, len(text), 0x12, 0, 1)
    symtab = sym_null + sym_start + sym_main

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
    ident[4] = 1  # ELFCLASS32
    ident[5] = 1  # ELFDATA2LSB
    ident[6] = 1  # EV_CURRENT

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
        sh(sh_name[".rodata"], 1, 0x2, BASE + 0x20, rodata_off, len(rodata), addralign=4),
        sh(sh_name[".shstrtab"], 3, 0, 0, shstr_off, len(shstr)),
        sh(sh_name[".strtab"], 3, 0, 0, strtab_off, len(strtab)),
        # link=4 (.strtab), info=1 (one local symbol: null), entsize=16
        sh(sh_name[".symtab"], 2, 0, 0, symtab_off, len(symtab), link=4, info=1,
           addralign=4, entsize=16),
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
