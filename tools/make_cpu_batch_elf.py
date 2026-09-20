#!/usr/bin/env python3
"""Generate a compact SH-4 ELF covering the 0.0.15 integer/system batch.

The program touches TAS.B, T-bit control, MAC/FP special registers, multiply,
swap/xtrct, and dynamic shifts. It deliberately returns R0=42.
"""
from __future__ import annotations
import argparse, struct
from pathlib import Path

BASE=0x8C010000
DATA=0x8C020000
EM_SH=42; ET_EXEC=2; EV_CURRENT=1

def align(v,a): return (v+a-1)&~(a-1)

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('output', nargs='?', default='samples/sh4_cpu_batch.elf'); args=ap.parse_args()
    output=Path(args.output); output.parent.mkdir(parents=True, exist_ok=True)

    # First word is patched after the literal-pool offset is known.
    words=[
        0xD000,       # mov.l literal,r10 (patched to DAxx)
        0x4A1B,       # tas.b @r10 -> T=1 because byte starts at zero
        0x0B29,       # movt r11
        0x4A1B,       # tas.b @r10 -> T=0 because bit7 is now set
        0x0C29,       # movt r12
        0x0008,       # clrt
        0x0018,       # sett
        0x0028,       # clrmac
        0xE106,       # mov #6,r1
        0xE207,       # mov #7,r2
        0x0127,       # mul.l r2,r1 -> MACL=42
        0x001A,       # sts macl,r0 -> 42
        0xE301,       # mov #1,r3
        0x403D,       # shld r3,r0 -> 84
        0xE3FF,       # mov #-1,r3
        0x403D,       # shld r3,r0 -> 42
        0x405A,       # lds r0,fpul
        0xE000,       # mov #0,r0
        0x005A,       # sts fpul,r0 -> 42
        0x212E,       # mulu.w r2,r1
        0x212F,       # muls.w r2,r1
        0x3125,       # dmulu.l r2,r1
        0x312D,       # dmuls.l r2,r1
        0x6418,       # swap.b r1,r4
        0x6519,       # swap.w r1,r5
        0x261D,       # xtrct r1,r6
        0x410A,       # lds r1,mach
        0x070A,       # sts mach,r7
        0x421A,       # lds r2,macl
        0x081A,       # sts macl,r8
        0x436A,       # lds r3,fpscr
        0x096A,       # sts fpscr,r9
        0xE415,       # mov #21,r4
        0xE301,       # mov #1,r3
        0x443C,       # shad r3,r4 -> 42
        0x000B,       # rts
        0x0009,       # delay slot
    ]
    code_size=len(words)*2
    literal_off=align(code_size,4)
    disp=(literal_off-4)//4
    assert 0 <= disp <= 0xFF
    words[0]=0xDA00 | disp
    code=b''.join(struct.pack('<H',w) for w in words)
    text=code + b'\0'*(literal_off-len(code)) + struct.pack('<I',DATA)
    data=b'\x00\x00\x00\x00'

    shstr=b'\0.text\0.data\0.shstrtab\0.strtab\0.symtab\0'
    names=['.text','.data','.shstrtab','.strtab','.symtab']; sh_name={n:shstr.index(n.encode()) for n in names}
    strtab=b'\0_main\0_tas_byte\0'
    syms=[b'\0'*16]
    syms.append(struct.pack('<IIIBBH',strtab.index(b'_main'),BASE,code_size,0x12,0,1))
    syms.append(struct.pack('<IIIBBH',strtab.index(b'_tas_byte'),DATA,len(data),0x11,0,2))
    symtab=b''.join(syms)

    ehsize=52; shentsize=40; text_off=0x100; data_off=align(text_off+len(text),4); shstr_off=align(data_off+len(data),4); strtab_off=shstr_off+len(shstr); symtab_off=align(strtab_off+len(strtab),4); shoff=align(symtab_off+len(symtab),4); shnum=6; shstrndx=3
    ident=bytearray(16); ident[:4]=b'\x7fELF'; ident[4]=1; ident[5]=1; ident[6]=1
    ehdr=struct.pack('<16sHHIIIIIHHHHHH',bytes(ident),ET_EXEC,EM_SH,EV_CURRENT,BASE,0,shoff,0,ehsize,0,0,shentsize,shnum,shstrndx)
    def sh(name,typ,flags,addr,off,size,link=0,info=0,addralign=1,entsize=0): return struct.pack('<IIIIIIIIII',name,typ,flags,addr,off,size,link,info,addralign,entsize)
    sections=[
        sh(0,0,0,0,0,0),
        sh(sh_name['.text'],1,0x6,BASE,text_off,len(text),addralign=4),
        sh(sh_name['.data'],1,0x3,DATA,data_off,len(data),addralign=4),
        sh(sh_name['.shstrtab'],3,0,0,shstr_off,len(shstr)),
        sh(sh_name['.strtab'],3,0,0,strtab_off,len(strtab)),
        sh(sh_name['.symtab'],2,0,0,symtab_off,len(symtab),link=4,info=1,addralign=4,entsize=16),
    ]
    blob=bytearray(shoff+shnum*shentsize); blob[:len(ehdr)]=ehdr
    blob[text_off:text_off+len(text)]=text; blob[data_off:data_off+len(data)]=data; blob[shstr_off:shstr_off+len(shstr)]=shstr; blob[strtab_off:strtab_off+len(strtab)]=strtab; blob[symtab_off:symtab_off+len(symtab)]=symtab
    for i,h in enumerate(sections): blob[shoff+i*shentsize:shoff+(i+1)*shentsize]=h
    output.write_bytes(blob)
    print(f'Wrote {output} ({len(blob)} bytes, literal @ 0x{BASE+literal_off:08X})')
if __name__=='__main__': main()
