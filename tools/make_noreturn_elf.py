#!/usr/bin/env python3
"""Generate a tiny ELF32/SH fixture for noreturn CFG pruning.

_main calls _abort and then contains words that deliberately decode as unknown/data.
A correct function CFG must stop after the call delay slot instead of treating the
post-abort bytes as executable fallthrough.
"""
from __future__ import annotations
import argparse, struct
from pathlib import Path

BASE=0x8C010000
ABORT=BASE+0x10
EM_SH=42; ET_EXEC=2; EV_CURRENT=1

def align(v,a): return (v+a-1)&~(a-1)

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('output', nargs='?', default='samples/sh4_noreturn.elf'); args=ap.parse_args()
    output=Path(args.output); output.parent.mkdir(parents=True, exist_ok=True)
    # BSR target = PC+4 + disp*2. 0x10 = 4 + 6*2 -> B006.
    main_words=[
        0xB006, # bsr _abort
        0x0009, # delay slot
        0xFFFF, # data / deliberately unknown if false fallthrough is followed
        0x8C01, # address-like data
        0xFFFF,
        0x8C01,
    ]
    # pad to 0x10, then a tiny body for symbol resolution only
    text=b''.join(struct.pack('<H',w) for w in main_words)
    text += b'\x00'*(0x10-len(text))
    text += struct.pack('<HH',0x000B,0x0009) # _abort body (never executed by this test)

    shstr=b'\0.text\0.shstrtab\0.strtab\0.symtab\0'; names=['.text','.shstrtab','.strtab','.symtab']; sh_name={n:shstr.index(n.encode()) for n in names}
    strtab=b'\0_main\0_abort\0'
    def sym(name,value,size,info): return struct.pack('<IIIBBH',strtab.index(name.encode()),value,size,info,0,1)
    symtab=b'\0'*16 + sym('_main',BASE,0x10,0x12) + sym('_abort',ABORT,4,0x12)
    ehsize=52; shentsize=40; text_off=0x100; shstr_off=align(text_off+len(text),4); strtab_off=shstr_off+len(shstr); symtab_off=align(strtab_off+len(strtab),4); shoff=align(symtab_off+len(symtab),4); shnum=5; shstrndx=2
    ident=bytearray(16); ident[:4]=b'\x7fELF'; ident[4]=1; ident[5]=1; ident[6]=1
    ehdr=struct.pack('<16sHHIIIIIHHHHHH',bytes(ident),ET_EXEC,EM_SH,EV_CURRENT,BASE,0,shoff,0,ehsize,0,0,shentsize,shnum,shstrndx)
    def sh(name,typ,flags,addr,off,size,link=0,info=0,addralign=1,entsize=0): return struct.pack('<IIIIIIIIII',name,typ,flags,addr,off,size,link,info,addralign,entsize)
    sections=[sh(0,0,0,0,0,0),sh(sh_name['.text'],1,0x6,BASE,text_off,len(text),addralign=2),sh(sh_name['.shstrtab'],3,0,0,shstr_off,len(shstr)),sh(sh_name['.strtab'],3,0,0,strtab_off,len(strtab)),sh(sh_name['.symtab'],2,0,0,symtab_off,len(symtab),link=3,info=1,addralign=4,entsize=16)]
    data=bytearray(shoff+shnum*shentsize); data[:len(ehdr)]=ehdr; data[text_off:text_off+len(text)]=text; data[shstr_off:shstr_off+len(shstr)]=shstr; data[strtab_off:strtab_off+len(strtab)]=strtab; data[symtab_off:symtab_off+len(symtab)]=symtab
    for i,h in enumerate(sections): data[shoff+i*shentsize:shoff+(i+1)*shentsize]=h
    output.write_bytes(data); print(f'Wrote {output} ({len(data)} bytes)')
if __name__=='__main__': main()
