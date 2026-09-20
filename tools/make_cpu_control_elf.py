#!/usr/bin/env python3
"""Generate SH-4 ELF for DreamcastRecomp 0.0.19 CPU-control tests.

Exercises DIV0U/DIV0S/DIV1, CMP/STR, BSRF and BRAF.  The dynamic branch/call
register is deliberately changed in each delay slot to verify that the target
is sampled before the slot executes.  Program returns R0=42.
"""
from __future__ import annotations
import argparse, struct
from pathlib import Path
BASE=0x8C010000; EM_SH=42; ET_EXEC=2; EV_CURRENT=1

def align(v,a): return (v+a-1)&~(a-1)

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('output', nargs='?', default='samples/sh4_cpu_control.elf'); a=ap.parse_args()
    out=Path(a.output); out.parent.mkdir(parents=True, exist_ok=True)
    # _main: the BT is taken at runtime, but its untaken path contains a BSR so
    # static program analysis also discovers _helper.  Runtime then calls the
    # same helper through BSRF.
    main_words=[
        0x0018,       # sett
        0x8905,       # bt 0x8C010010
        0xB01C,       # bsr _helper @ 0x8C010040 (analysis-only path)
        0x0009,       # delay slot
        0xE0FF,       # mov #-1,r0
        0x000B,       # rts
        0x0009,       # delay slot
        0x0009,       # padding
        0x4F22,       # sts.l pr,@-r15 (preserve host return PR)
        0xE428,       # mov #40,r4 ; helper - (BSRF_PC+4)
        0x0403,       # bsrf r4
        0xE400,       # delay slot: mov #0,r4 (target must already be sampled)
        0x4F26,       # lds.l @r15+,pr
        0x882A,       # cmp/eq #42,r0
        0x8902,       # bt success @ 0x8C010024
        0xE0FF,       # mov #-1,r0
        0x000B,       # rts
        0x0009,       # delay slot
        0x000B,       # success: rts
        0x0009,       # delay slot
    ]
    assert len(main_words)*2 == 0x28
    # Pad to helper at +0x40.
    words=main_words + [0x0009]*((0x40-len(main_words)*2)//2)
    helper_words=[
        0xE5FF,       # mov #-1,r5
        0xE601,       # mov #1,r6
        0x2567,       # div0s r6,r5 -> T=1
        0x0729,       # movt r7
        0xE10A,       # mov #10,r1
        0xE203,       # mov #3,r2
        0x0019,       # div0u
        0x3124,       # div1 r2,r1 -> r1=17,T=1
        0x6013,       # mov r1,r0
        0x8811,       # cmp/eq #17,r0
        0x0329,       # movt r3
        0xE028,       # mov #40,r0
        0x307C,       # add r7,r0 -> 41
        0x303C,       # add r3,r0 -> 42
        0xE500,       # mov #0,r5
        0xE600,       # mov #0,r6
        0x256C,       # cmp/str r6,r5 -> T=1
        0x8B03,       # bf target (not taken, but makes target a CFG leader)
        0xE402,       # mov #2,r4
        0x0423,       # braf r4 -> target @ +0x6C
        0xE400,       # delay slot: mov #0,r4 (target must already be sampled)
        0xE0FF,       # bad: mov #-1,r0
        0x000B,       # target: rts
        0x0009,       # delay slot
    ]
    words += helper_words
    text=b''.join(struct.pack('<H',w) for w in words)
    main_size=0x28; helper_addr=BASE+0x40; helper_size=len(helper_words)*2

    shstr=b'\0.text\0.shstrtab\0.strtab\0.symtab\0'; names=['.text','.shstrtab','.strtab','.symtab']; shn={n:shstr.index(n.encode()) for n in names}
    strtab=b'\0_main\0_helper\0'
    syms=[b'\0'*16,
          struct.pack('<IIIBBH',strtab.index(b'_main'),BASE,main_size,0x12,0,1),
          struct.pack('<IIIBBH',strtab.index(b'_helper'),helper_addr,helper_size,0x12,0,1)]
    symtab=b''.join(syms)
    ehsize=52; shentsize=40; text_off=0x100; shstr_off=align(text_off+len(text),4); strtab_off=shstr_off+len(shstr); symtab_off=align(strtab_off+len(strtab),4); shoff=align(symtab_off+len(symtab),4); shnum=5; shstrndx=2
    ident=bytearray(16); ident[:4]=b'\x7fELF'; ident[4]=1; ident[5]=1; ident[6]=1
    ehdr=struct.pack('<16sHHIIIIIHHHHHH',bytes(ident),ET_EXEC,EM_SH,EV_CURRENT,BASE,0,shoff,0,ehsize,0,0,shentsize,shnum,shstrndx)
    def sh(name,typ,flags,addr,off,size,link=0,info=0,addralign=1,entsize=0): return struct.pack('<IIIIIIIIII',name,typ,flags,addr,off,size,link,info,addralign,entsize)
    sections=[sh(0,0,0,0,0,0), sh(shn['.text'],1,0x6,BASE,text_off,len(text),addralign=4), sh(shn['.shstrtab'],3,0,0,shstr_off,len(shstr)), sh(shn['.strtab'],3,0,0,strtab_off,len(strtab)), sh(shn['.symtab'],2,0,0,symtab_off,len(symtab),link=3,info=1,addralign=4,entsize=16)]
    blob=bytearray(shoff+shnum*shentsize); blob[:len(ehdr)]=ehdr; blob[text_off:text_off+len(text)]=text; blob[shstr_off:shstr_off+len(shstr)]=shstr; blob[strtab_off:strtab_off+len(strtab)]=strtab; blob[symtab_off:symtab_off+len(symtab)]=symtab
    for i,h in enumerate(sections): blob[shoff+i*shentsize:shoff+(i+1)*shentsize]=h
    out.write_bytes(blob); print(f'Wrote {out} ({len(blob)} bytes)')
if __name__=='__main__': main()
