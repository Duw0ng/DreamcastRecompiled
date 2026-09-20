#!/usr/bin/env python3
"""Generate a small ELF32/SH program for DreamcastRecomp 0.0.8 control-flow tests.

Call graph:
    _main --BSR--> _branch_loop

_branch_loop computes 5+4+3+2+1 using DT/BF, checks that the result is 15
with CMP/EQ + BT, and returns 42 on success. This intentionally exercises:
  * a backward conditional branch (loop), including taken and not-taken paths
  * T-bit writes via DT and CMP/EQ
  * BT/BF conditional edges
  * BRA + delay slot
  * a recompiled function call and return
"""
from __future__ import annotations

import argparse
import struct
from pathlib import Path

BASE = 0x8C010000
MAIN = BASE
BRANCH_LOOP = BASE + 0x40
EM_SH = 42
ET_EXEC = 2
EV_CURRENT = 1


def align(value: int, alignment: int) -> int:
    return (value + alignment - 1) & ~(alignment - 1)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("output", nargs="?", default="samples/sh4_branches.elf")
    args = ap.parse_args()
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)

    # _main @ BASE. BSR at +2 targets BASE+0x40:
    # disp = (0x40 - (0x02 + 4)) / 2 = 0x1D.
    main_words = [
        0x4F22,  # sts.l pr,@-r15
        0xB01D,  # bsr _branch_loop
        0x0009,  # delay slot
        0x4F26,  # lds.l @r15+,pr
        0x000B,  # rts
        0x0009,  # delay slot
    ]
    main_code = b"".join(struct.pack("<H", w) for w in main_words)
    assert len(main_code) == 12

    # _branch_loop @ BASE+0x40
    # R1 = loop counter 5; R0 = sum.
    # loop: R0 += R1; DT R1; BF loop
    # result must be 15; BT success; otherwise -1; success returns 42.
    branch_words = [
        0xE105,  # +0x00 mov #5,r1
        0xE000,  # +0x02 mov #0,r0
        0x301C,  # +0x04 add r1,r0        [loop]
        0x4110,  # +0x06 dt r1            T=(--r1 == 0)
        0x8BFC,  # +0x08 bf loop          target +0x04
        0x880F,  # +0x0A cmp/eq #15,r0    T=(r0 == 15)
        0x8902,  # +0x0C bt success       target +0x14
        0xE0FF,  # +0x0E mov #-1,r0       failure path
        0xA001,  # +0x10 bra done          target +0x16
        0x0009,  # +0x12 delay slot
        0xE02A,  # +0x14 mov #42,r0       [success]
        0x000B,  # +0x16 rts              [done]
        0x0009,  # +0x18 delay slot
    ]
    branch_code = b"".join(struct.pack("<H", w) for w in branch_words)
    assert len(branch_code) == 26

    text = bytearray(0x40 + len(branch_code))
    text[0:len(main_code)] = main_code
    for off in range(len(main_code), 0x40, 2):
        text[off:off+2] = struct.pack("<H", 0x0009)
    text[0x40:0x40+len(branch_code)] = branch_code

    shstr = b"\0.text\0.shstrtab\0.strtab\0.symtab\0"
    names = [".text", ".shstrtab", ".strtab", ".symtab"]
    sh_name = {name: shstr.index(name.encode()) for name in names}
    strtab = b"\0_main\0_branch_loop\0"

    sym_null = b"\0" * 16
    sym_main = struct.pack("<IIIBBH", strtab.index(b"_main"), MAIN, len(main_code), 0x12, 0, 1)
    sym_branch = struct.pack("<IIIBBH", strtab.index(b"_branch_loop"), BRANCH_LOOP, len(branch_code), 0x12, 0, 1)
    symtab = sym_null + sym_main + sym_branch

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
