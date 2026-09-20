#!/usr/bin/env python3
"""Generate a compact SH-4 ELF covering DreamcastRecomp 0.0.19 FMOV forms.

The program moves raw IEEE-754 bit patterns between Dreamcast RAM and FR
registers using every Fnm6-FnmC FMOV addressing form implemented in 0.0.19,
verifies the memory results using integer MOV.L/CMP/EQ, and returns R0=42.
"""
from __future__ import annotations
import argparse, struct
from pathlib import Path

BASE=0x8C010000
DATA=0x8C020000
EM_SH=42; ET_EXEC=2; EV_CURRENT=1

def align(v,a): return (v+a-1)&~(a-1)

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('output', nargs='?', default='samples/sh4_fpu_moves.elf'); args=ap.parse_args()
    output=Path(args.output); output.parent.mkdir(parents=True, exist_ok=True)

    words=[]
    labels={}
    fix_bf=[]
    def emit(w): words.append(w)
    def label(name): labels[name]=len(words)*2
    def bf(name): fix_bf.append((len(words),name)); emit(0x8B00)

    emit(0xD400)       # mov.l literal,r4 (patched)
    emit(0x6543)       # mov r4,r5
    emit(0x7504)       # add #4,r5
    emit(0x6643)       # mov r4,r6
    emit(0x7610)       # add #16,r6 -> output[0]
    emit(0x6763)       # mov r6,r7
    emit(0x7708)       # add #8,r7 -> output[2]+4, predec => output[1]
    emit(0x6863)       # mov r6,r8
    emit(0x7808)       # add #8,r8 -> output[2]

    emit(0xF148)       # fmov.s @r4,fr1
    emit(0xF21C)       # fmov fr1,fr2
    emit(0xF62A)       # fmov.s fr2,@r6
    emit(0xF359)       # fmov.s @r5+,fr3
    emit(0xF73B)       # fmov.s fr3,@-r7
    emit(0xE000)       # mov #0,r0
    emit(0xF446)       # fmov.s @(r0,r4),fr4
    emit(0xF847)       # fmov.s fr4,@(r0,r8)

    # Verify output[0] == source[0].
    emit(0x6142)       # mov.l @r4,r1
    emit(0x6262)       # mov.l @r6,r2
    emit(0x3210)       # cmp/eq r1,r2
    bf('fail')

    # Verify output[1] == source[1].
    emit(0x6943)       # mov r4,r9
    emit(0x7904)       # add #4,r9
    emit(0x6192)       # mov.l @r9,r1
    emit(0x6A63)       # mov r6,r10
    emit(0x7A04)       # add #4,r10
    emit(0x62A2)       # mov.l @r10,r2
    emit(0x3210)       # cmp/eq r1,r2
    bf('fail')

    # Verify output[2] == source[0] (indexed FMOV pair).
    emit(0x6142)       # mov.l @r4,r1
    emit(0x6282)       # mov.l @r8,r2
    emit(0x3210)       # cmp/eq r1,r2
    bf('fail')

    # Verify @Rm+ advanced by four bytes.
    emit(0x6A43)       # mov r4,r10
    emit(0x7A08)       # add #8,r10
    emit(0x35A0)       # cmp/eq r10,r5
    bf('fail')

    # Verify @-Rn decremented by four bytes to output[1].
    emit(0x6A63)       # mov r6,r10
    emit(0x7A04)       # add #4,r10
    emit(0x37A0)       # cmp/eq r10,r7
    bf('fail')

    emit(0xE02A)       # mov #42,r0
    emit(0x000B)       # rts
    emit(0x0009)       # delay slot
    label('fail')
    emit(0xE0FF)       # mov #-1,r0
    emit(0x000B)
    emit(0x0009)

    # Patch BF displacements.
    for idx,name in fix_bf:
        addr=idx*2
        target=labels[name]
        delta=target-(addr+4)
        assert delta % 2 == 0
        disp=delta//2
        assert -128 <= disp <= 127
        words[idx]=0x8B00 | (disp & 0xFF)

    code_size=len(words)*2
    literal_off=align(code_size,4)
    # MOV.L at BASE: EA = (PC+4)&~3 + disp*4.
    disp=(literal_off-4)//4
    assert 0 <= disp <= 0xFF
    words[0]=0xD400 | disp
    code=b''.join(struct.pack('<H',w) for w in words)
    text=code + b'\0'*(literal_off-len(code)) + struct.pack('<I',DATA)

    data=struct.pack('<IIIIIII',
        0x3F800000, # 1.0f
        0x40000000, # 2.0f
        0x40400000, # 3.0f (spare)
        0,
        0,0,0       # outputs
    )

    shstr=b'\0.text\0.data\0.shstrtab\0.strtab\0.symtab\0'
    names=['.text','.data','.shstrtab','.strtab','.symtab']; sh_name={n:shstr.index(n.encode()) for n in names}
    strtab=b'\0_main\0_fpu_data\0'
    syms=[b'\0'*16]
    syms.append(struct.pack('<IIIBBH',strtab.index(b'_main'),BASE,code_size,0x12,0,1))
    syms.append(struct.pack('<IIIBBH',strtab.index(b'_fpu_data'),DATA,len(data),0x11,0,2))
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
    print(f'Wrote {output} ({len(blob)} bytes, {len(words)} SH-4 words, literal @ 0x{BASE+literal_off:08X})')

if __name__=='__main__': main()
