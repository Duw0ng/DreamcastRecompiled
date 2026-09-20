#!/usr/bin/env python3
"""Generate a tiny ELF32/SH program for NOT Rm,Rn regression testing.

_main computes ~(-43) == 42 and returns. The result is intentionally visible in
R0 so the generated native runner can validate the full SH-4 -> DCIR -> C++ path.
"""
from __future__ import annotations
import argparse, struct
from pathlib import Path

BASE=0x8C010000
EM_SH=42; ET_EXEC=2; EV_CURRENT=1

def align(v,a): return (v+a-1)&~(a-1)

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('output', nargs='?', default='samples/sh4_not.elf'); args=ap.parse_args()
    output=Path(args.output); output.parent.mkdir(parents=True, exist_ok=True)
    words=[
        0xE1D5, # mov #-43,r1
        0x6017, # not r1,r0 -> 42
        0x000B, # rts
        0x0009, # delay slot
    ]
    text=b''.join(struct.pack('<H',w) for w in words)
    shstr=b'\0.text\0.shstrtab\0.strtab\0.symtab\0'; names=['.text','.shstrtab','.strtab','.symtab']; sh_name={n:shstr.index(n.encode()) for n in names}
    strtab=b'\0_main\0'
    symtab=b'\0'*16 + struct.pack('<IIIBBH',strtab.index(b'_main'),BASE,len(text),0x12,0,1)
    ehsize=52; shentsize=40; text_off=0x100; shstr_off=align(text_off+len(text),4); strtab_off=shstr_off+len(shstr); symtab_off=align(strtab_off+len(strtab),4); shoff=align(symtab_off+len(symtab),4); shnum=5; shstrndx=2
    ident=bytearray(16); ident[:4]=b'\x7fELF'; ident[4]=1; ident[5]=1; ident[6]=1
    ehdr=struct.pack('<16sHHIIIIIHHHHHH',bytes(ident),ET_EXEC,EM_SH,EV_CURRENT,BASE,0,shoff,0,ehsize,0,0,shentsize,shnum,shstrndx)
    def sh(name,typ,flags,addr,off,size,link=0,info=0,addralign=1,entsize=0): return struct.pack('<IIIIIIIIII',name,typ,flags,addr,off,size,link,info,addralign,entsize)
    sections=[sh(0,0,0,0,0,0),sh(sh_name['.text'],1,0x6,BASE,text_off,len(text),addralign=2),sh(sh_name['.shstrtab'],3,0,0,shstr_off,len(shstr)),sh(sh_name['.strtab'],3,0,0,strtab_off,len(strtab)),sh(sh_name['.symtab'],2,0,0,symtab_off,len(symtab),link=3,info=1,addralign=4,entsize=16)]
    data=bytearray(shoff+shnum*shentsize); data[:len(ehdr)]=ehdr; data[text_off:text_off+len(text)]=text; data[shstr_off:shstr_off+len(shstr)]=shstr; data[strtab_off:strtab_off+len(strtab)]=strtab; data[symtab_off:symtab_off+len(symtab)]=symtab
    for i,h in enumerate(sections): data[shoff+i*shentsize:shoff+(i+1)*shentsize]=h
    output.write_bytes(data); print(f'Wrote {output} ({len(data)} bytes)')
if __name__=='__main__': main()
