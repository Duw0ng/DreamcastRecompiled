#!/usr/bin/env python3
"""Generate SH-4 ELF with a cross-symbol branch into a shared function tail.

_main BSRs _signed. _signed saves R4 then BRA's into _shared+8, where the
shared epilogue restores R4 and returns through the PR established by _main.
This mirrors libgcc helpers that branch between adjacent function symbols.
"""
from __future__ import annotations
import argparse, struct
from pathlib import Path

BASE=0x8C010000; MAIN=BASE; SHARED=BASE+0x40; SIGNED=BASE+0x80
EM_SH=42; ET_EXEC=2; EV_CURRENT=1

def align(v,a): return (v+a-1)&~(a-1)
def bra(src,target):
    disp=(target-(src+4))//2
    assert -2048 <= disp <= 2047
    return 0xA000 | (disp & 0x0FFF)
def bsr(src,target):
    disp=(target-(src+4))//2
    assert -2048 <= disp <= 2047
    return 0xB000 | (disp & 0x0FFF)

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('output',nargs='?',default='samples/sh4_cross_branch.elf'); a=ap.parse_args()
    out=Path(a.output); out.parent.mkdir(parents=True,exist_ok=True)
    main_words=[0x4F22, bsr(MAIN+2,SIGNED),0x0009,0x4F26,0x000B,0x0009]
    # Normal _shared entry also reaches its own shared epilogue at +8.
    shared_words=[0x2F46,0xE007,bra(SHARED+4,SHARED+8),0x0009,0x64F6,0x000B,0x0009]
    signed_words=[0x2F46,0xE02A,bra(SIGNED+4,SHARED+8),0x0009]
    text=bytearray(0x88)
    for off in range(0,len(text),2): text[off:off+2]=struct.pack('<H',0x0009)
    for addr,words in [(MAIN,main_words),(SHARED,shared_words),(SIGNED,signed_words)]:
        off=addr-BASE; blob=b''.join(struct.pack('<H',w) for w in words); text[off:off+len(blob)]=blob
    shstr=b'\0.text\0.shstrtab\0.strtab\0.symtab\0'; names=['.text','.shstrtab','.strtab','.symtab']; shn={n:shstr.index(n.encode()) for n in names}
    strtab=b'\0_main\0_shared\0_signed\0'
    syms=b'\0'*16
    for name,addr,size in [('_main',MAIN,len(main_words)*2),('_shared',SHARED,len(shared_words)*2),('_signed',SIGNED,len(signed_words)*2)]:
        syms += struct.pack('<IIIBBH',strtab.index(name.encode()),addr,size,0x12,0,1)
    ehsize=52; shentsize=40; text_off=0x100; shstr_off=align(text_off+len(text),4); strtab_off=shstr_off+len(shstr); symtab_off=align(strtab_off+len(strtab),4); shoff=align(symtab_off+len(syms),4); shnum=5
    ident=bytearray(16); ident[:4]=b'\x7fELF'; ident[4]=1; ident[5]=1; ident[6]=1
    ehdr=struct.pack('<16sHHIIIIIHHHHHH',bytes(ident),ET_EXEC,EM_SH,EV_CURRENT,MAIN,0,shoff,0,ehsize,0,0,shentsize,shnum,2)
    def sh(name,typ,flags,addr,off,size,link=0,info=0,addralign=1,entsize=0): return struct.pack('<IIIIIIIIII',name,typ,flags,addr,off,size,link,info,addralign,entsize)
    secs=[sh(0,0,0,0,0,0),sh(shn['.text'],1,0x6,BASE,text_off,len(text),addralign=2),sh(shn['.shstrtab'],3,0,0,shstr_off,len(shstr)),sh(shn['.strtab'],3,0,0,strtab_off,len(strtab)),sh(shn['.symtab'],2,0,0,symtab_off,len(syms),link=3,info=1,addralign=4,entsize=16)]
    data=bytearray(shoff+shnum*shentsize); data[:len(ehdr)]=ehdr; data[text_off:text_off+len(text)]=text; data[shstr_off:shstr_off+len(shstr)]=shstr; data[strtab_off:strtab_off+len(strtab)]=strtab; data[symtab_off:symtab_off+len(syms)]=syms
    for i,h in enumerate(secs): data[shoff+i*shentsize:shoff+(i+1)*shentsize]=h
    out.write_bytes(data); print(f'Wrote {out} ({len(data)} bytes)')
if __name__=='__main__': main()
