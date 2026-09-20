#!/usr/bin/env python3
"""Generate a compact SH-4 ELF for DreamcastRecomp 0.0.19 FPU arithmetic.

Covers single-precision FADD/FSUB/FMUL/FDIV/FCMP/EQ/FCMP/GT/FMAC and a
PR=1 double-precision FADD + FCMP path. Results are written to Dreamcast RAM,
verified using integer loads/comparisons, and the function returns R0=42.
"""
from __future__ import annotations
import argparse, struct
from pathlib import Path

BASE=0x8C010000
DATA=0x8C020000
EM_SH=42; ET_EXEC=2; EV_CURRENT=1

def align(v,a): return (v+a-1)&~(a-1)

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('output', nargs='?', default='samples/sh4_fpu_arith.elf'); args=ap.parse_args()
    output=Path(args.output); output.parent.mkdir(parents=True, exist_ok=True)
    words=[]; labels={}; fix_bf=[]; fix_bt=[]; literals=[]
    def emit(w): words.append(w)
    def label(n): labels[n]=len(words)*2
    def bf(n): fix_bf.append((len(words),n)); emit(0x8B00)
    def bt(n): fix_bt.append((len(words),n)); emit(0x8900)
    def movl_pc(rn, value):
        # patched after code layout; record instruction index, register and value
        idx=len(words); emit(0xD000 | (rn<<8)); literals.append((idx,rn,value))

    # r4 = DATA. r6 = DATA+24 (single outputs).
    movl_pc(4, DATA)
    emit(0x6643)       # mov r4,r6
    emit(0x7618)       # add #24,r6

    # Load FR0=2, FR1=3, FR2=4, FR3=8 from DATA.
    emit(0x6543)       # mov r4,r5
    emit(0xF059)       # fmov.s @r5+,fr0
    emit(0xF159)       # fmov.s @r5+,fr1
    emit(0xF259)       # fmov.s @r5+,fr2
    emit(0xF358)       # fmov.s @r5,fr3

    # FADD: 4 + 3 = 7 -> output[0].
    emit(0xF210)       # fadd fr1,fr2
    emit(0xF62A)       # fmov.s fr2,@r6
    emit(0x7604)       # add #4,r6

    # Reload FR2=4; FSUB: 4 - 3 = 1 -> output[1].
    emit(0x6543); emit(0x7508); emit(0xF258)
    emit(0xF211)       # fsub fr1,fr2
    emit(0xF62A); emit(0x7604)

    # Reload FR2=4; FMUL: 4 * 3 = 12 -> output[2].
    emit(0x6543); emit(0x7508); emit(0xF258)
    emit(0xF212)       # fmul fr1,fr2
    emit(0xF62A); emit(0x7604)

    # FDIV: FR3(8) / FR0(2) = 4 -> output[3].
    emit(0xF303)       # fdiv fr0,fr3
    emit(0xF63A); emit(0x7604)

    # Reload FR2=4. Validate FCMP/EQ false against FR1=3.
    emit(0x6543); emit(0x7508); emit(0xF258)
    emit(0xF214)       # fcmp/eq fr1,fr2 => false
    bt('fail')          # unexpected equality => fail

    # FCMP/GT: FR2(4) > FR1(3) => true.
    emit(0xF215)
    bf('fail')          # BF means failure if false

    # FMAC: FR2 = FR0(2)*FR1(3)+FR2(4) = 10 -> output[4].
    emit(0xF21E)
    emit(0xF62A)

    # Double operands at DATA+64: 1.5 then 2.0. Load low word into odd FR,
    # high word into even FR so DR2={FR2,FR3}, DR4={FR4,FR5}.
    emit(0x6543); emit(0x7540)       # mov r4,r5; add #64,r5
    emit(0xF359); emit(0xF259)       # DR2 = 1.5
    emit(0xF559); emit(0xF459)       # DR4 = 2.0
    movl_pc(1, 0x00080000)           # FPSCR.PR
    emit(0x416A)                     # lds r1,fpscr
    emit(0xF240)                     # fadd dr4,dr2 => 3.5
    emit(0xF245)                     # fcmp/gt dr4,dr2 => true
    bf('fail')
    emit(0xF224)                     # fcmp/eq dr2,dr2 => true
    bf('fail')

    # Store DR2 as memory-order low/high words at DATA+48.
    emit(0x6643); emit(0x7630)       # r6=DATA+48
    emit(0xF63A)                     # fr3 (low) -> @r6
    emit(0x7604)
    emit(0xF62A)                     # fr2 (high) -> @r6
    emit(0xE100); emit(0x416A)       # fpscr=0 again

    # Verify five single results using expected values at DATA+80.
    # r6=DATA+24, r7=DATA+80, r1 loop count=5.
    emit(0x6643); emit(0x7618)
    emit(0x6743); emit(0x7750)
    emit(0xE105)
    label('verify_single')
    emit(0x6266)       # mov.l @r6+,r2
    emit(0x6376)       # mov.l @r7+,r3
    emit(0x3230)       # cmp/eq r3,r2
    bf('fail')
    emit(0x4110)       # dt r1
    bf('verify_single')

    # Verify double output low/high against expected at DATA+100.
    emit(0x6643); emit(0x7630)       # r6=DATA+48
    emit(0x6743); emit(0x7764)       # r7=DATA+100
    emit(0x6266); emit(0x6376); emit(0x3230); bf('fail')
    emit(0x6262); emit(0x6372); emit(0x3230); bf('fail')

    emit(0xE02A); emit(0x000B); emit(0x0009)
    label('fail')
    emit(0xE0FF); emit(0x000B); emit(0x0009)

    # Patch conditional branch displacements.
    for idx,name in fix_bf:
        target=labels[name]; addr=idx*2; delta=target-(addr+4); assert delta%2==0
        disp=delta//2; assert -128<=disp<=127
        words[idx]=0x8B00 | (disp & 0xFF)
    for idx,name in fix_bt:
        target=labels[name]; addr=idx*2; delta=target-(addr+4); assert delta%2==0
        disp=delta//2; assert -128<=disp<=127
        words[idx]=0x8900 | (disp & 0xFF)

    code_size=len(words)*2
    lit_off=align(code_size,4)
    # Deduplicate literal values while preserving order.
    vals=[]
    for _,_,v in literals:
        if v not in vals: vals.append(v)
    value_off={v:lit_off+i*4 for i,v in enumerate(vals)}
    for idx,rn,v in literals:
        pc=BASE+idx*2
        ea_base=(pc+4)&~3
        disp=(BASE+value_off[v]-ea_base)//4
        assert 0<=disp<=0xFF
        words[idx]=0xD000 | (rn<<8) | disp
    code=b''.join(struct.pack('<H',w) for w in words)
    text=code+b'\0'*(lit_off-len(code))+b''.join(struct.pack('<I',v) for v in vals)

    # Data: sources [2,3,4,8], padding to +24, outputs[5], padding to +48,
    # double output[2], padding to +64, double operands, expected singles @+80,
    # padding, expected double @+100.
    data=bytearray(108)
    def w32(off,v): struct.pack_into('<I',data,off,v)
    for off,v in [(0,0x40000000),(4,0x40400000),(8,0x40800000),(12,0x41000000)]: w32(off,v)
    # Double operands, memory low then high.
    w32(64,0x00000000); w32(68,0x3FF80000)  # 1.5
    w32(72,0x00000000); w32(76,0x40000000)  # 2.0
    expected=[0x40E00000,0x3F800000,0x41400000,0x40800000,0x41200000]
    for i,v in enumerate(expected): w32(80+i*4,v)
    w32(100,0x00000000); w32(104,0x400C0000) # 3.5 double low/high

    shstr=b'\0.text\0.data\0.shstrtab\0.strtab\0.symtab\0'; names=['.text','.data','.shstrtab','.strtab','.symtab']; sh_name={n:shstr.index(n.encode()) for n in names}
    strtab=b'\0_main\0_fpu_data\0'; syms=[b'\0'*16, struct.pack('<IIIBBH',strtab.index(b'_main'),BASE,code_size,0x12,0,1), struct.pack('<IIIBBH',strtab.index(b'_fpu_data'),DATA,len(data),0x11,0,2)]; symtab=b''.join(syms)
    ehsize=52; shentsize=40; text_off=0x100; data_off=align(text_off+len(text),4); shstr_off=align(data_off+len(data),4); strtab_off=shstr_off+len(shstr); symtab_off=align(strtab_off+len(strtab),4); shoff=align(symtab_off+len(symtab),4); shnum=6; shstrndx=3
    ident=bytearray(16); ident[:4]=b'\x7fELF'; ident[4]=1; ident[5]=1; ident[6]=1
    ehdr=struct.pack('<16sHHIIIIIHHHHHH',bytes(ident),ET_EXEC,EM_SH,EV_CURRENT,BASE,0,shoff,0,ehsize,0,0,shentsize,shnum,shstrndx)
    def sh(name,typ,flags,addr,off,size,link=0,info=0,addralign=1,entsize=0): return struct.pack('<IIIIIIIIII',name,typ,flags,addr,off,size,link,info,addralign,entsize)
    sections=[sh(0,0,0,0,0,0),sh(sh_name['.text'],1,0x6,BASE,text_off,len(text),addralign=4),sh(sh_name['.data'],1,0x3,DATA,data_off,len(data),addralign=4),sh(sh_name['.shstrtab'],3,0,0,shstr_off,len(shstr)),sh(sh_name['.strtab'],3,0,0,strtab_off,len(strtab)),sh(sh_name['.symtab'],2,0,0,symtab_off,len(symtab),link=4,info=1,addralign=4,entsize=16)]
    blob=bytearray(shoff+shnum*shentsize); blob[:len(ehdr)]=ehdr; blob[text_off:text_off+len(text)]=text; blob[data_off:data_off+len(data)]=data; blob[shstr_off:shstr_off+len(shstr)]=shstr; blob[strtab_off:strtab_off+len(strtab)]=strtab; blob[symtab_off:symtab_off+len(symtab)]=symtab
    for i,h in enumerate(sections): blob[shoff+i*shentsize:shoff+(i+1)*shentsize]=h
    output.write_bytes(blob)
    print(f'Wrote {output} ({len(blob)} bytes, {len(words)} SH-4 words, {len(vals)} literals)')

if __name__=='__main__': main()
