#!/usr/bin/env python3
"""Generate a small ELF32/SH program for DreamcastRecomp multi-function tests.

Call graph:
    _main --JSR via literal--> _helper --BSR--> _leaf

_leaf returns 41 in R0. _helper adds 1. _main returns the result, so the
native generated program must return to the host with R0 == 42.
"""
from __future__ import annotations

import argparse
import struct
from pathlib import Path

BASE = 0x8C010000
MAIN = BASE
HELPER = BASE + 0x40
LEAF = BASE + 0x80
EM_SH = 42
ET_EXEC = 2
EV_CURRENT = 1


def align(value: int, alignment: int) -> int:
    return (value + alignment - 1) & ~(alignment - 1)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("output", nargs="?", default="samples/sh4_multifunc.elf")
    args = ap.parse_args()
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)

    # _main @ 0x8C010000, size 20. The final 4 bytes are a literal containing
    # the address of _helper. MOV.L at PC 0 uses ((PC+4)&~3)+disp*4.
    main_words = [
        0xD003,  # mov.l @(12,pc),r0 -> literal @ MAIN+0x10
        0x4F22,  # sts.l pr,@-r15
        0x400B,  # jsr @r0
        0x0009,  # delay slot
        0x4F26,  # lds.l @r15+,pr
        0x000B,  # rts
        0x0009,  # delay slot
        0x0009,  # alignment / unreachable padding
    ]
    main_code = b"".join(struct.pack("<H", w) for w in main_words) + struct.pack("<I", HELPER)
    assert len(main_code) == 20

    # _helper @ +0x40. BSR is at +0x42. Target calculation is PC+4+disp*2,
    # so disp=(0x80-0x46)/2=0x1D.
    helper_words = [
        0x4F22,  # sts.l pr,@-r15
        0xB01D,  # bsr _leaf
        0x0009,  # delay slot
        0x7001,  # add #1,r0
        0x4F26,  # lds.l @r15+,pr
        0x000B,  # rts
        0x0009,  # delay slot
    ]
    helper_code = b"".join(struct.pack("<H", w) for w in helper_words)
    assert len(helper_code) == 14

    leaf_words = [
        0xE029,  # mov #41,r0
        0x000B,  # rts
        0x0009,  # delay slot
    ]
    leaf_code = b"".join(struct.pack("<H", w) for w in leaf_words)
    assert len(leaf_code) == 6

    text = bytearray(0x86)
    text[0:len(main_code)] = main_code
    for off in range(len(main_code), 0x40, 2):
        text[off:off+2] = struct.pack("<H", 0x0009)
    text[0x40:0x40+len(helper_code)] = helper_code
    for off in range(0x40+len(helper_code), 0x80, 2):
        text[off:off+2] = struct.pack("<H", 0x0009)
    text[0x80:0x80+len(leaf_code)] = leaf_code

    shstr = b"\0.text\0.shstrtab\0.strtab\0.symtab\0"
    names = [".text", ".shstrtab", ".strtab", ".symtab"]
    sh_name = {name: shstr.index(name.encode()) for name in names}
    strtab = b"\0_main\0_helper\0_leaf\0"

    sym_null = b"\0" * 16
    sym_main = struct.pack("<IIIBBH", strtab.index(b"_main"), MAIN, len(main_code), 0x12, 0, 1)
    sym_helper = struct.pack("<IIIBBH", strtab.index(b"_helper"), HELPER, len(helper_code), 0x12, 0, 1)
    sym_leaf = struct.pack("<IIIBBH", strtab.index(b"_leaf"), LEAF, len(leaf_code), 0x12, 0, 1)
    symtab = sym_null + sym_main + sym_helper + sym_leaf

    ehsize = 52
    shentsize = 40
    text_off = 0x100
    shstr_off = align(text_off + len(text), 4)
    strtab_off = shstr_off + len(shstr)
    symtab_off = align(strtab_off + len(strtab), 4)
    shoff = align(symtab_off + len(symtab), 4)
    shnum = 5
    shstrndx = 2

    ident = bytearray(16)
    ident[0:4] = b"\x7fELF"
    ident[4] = 1
    ident[5] = 1
    ident[6] = 1
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
        sh(sh_name[".text"], 1, 0x6, BASE, text_off, len(text), addralign=2),
        sh(sh_name[".shstrtab"], 3, 0, 0, shstr_off, len(shstr)),
        sh(sh_name[".strtab"], 3, 0, 0, strtab_off, len(strtab)),
        sh(sh_name[".symtab"], 2, 0, 0, symtab_off, len(symtab), link=3, info=1, addralign=4, entsize=16),
    ]

    total = shoff + shnum * shentsize
    data = bytearray(total)
    data[:len(ehdr)] = ehdr
    data[text_off:text_off+len(text)] = text
    data[shstr_off:shstr_off+len(shstr)] = shstr
    data[strtab_off:strtab_off+len(strtab)] = strtab
    data[symtab_off:symtab_off+len(symtab)] = symtab
    for i, header in enumerate(sections):
        begin = shoff + i * shentsize
        data[begin:begin+shentsize] = header

    output.write_bytes(data)
    print(f"Wrote {output} ({len(data)} bytes)")


if __name__ == "__main__":
    main()
